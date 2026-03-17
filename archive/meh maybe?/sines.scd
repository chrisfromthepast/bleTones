(
// 1. Enhanced SynthDef
SynthDef(\bleDynamic, {
	arg freq=440, amp=0, pan=0, sustain=10;
	var sig, env, lifeEnv;

	// Automatic expiry: after 'sustain' seconds, it kills itself
	lifeEnv = EnvGen.kr(Env.linen(0.1, sustain - 0.2, 0.1), doneAction: 2);

	// Twinkle vs Drone: add a high-pass sparkle for distant (high-freq) notes
	sig = Saw.ar([freq, freq * 1.005]) * 0.1;
	sig = sig + (SinOsc.ar(freq * 2) * 0.05); // Add a clear fundamental

	// Filter: Closer (low freq) = muddier/warmer, Distant (high freq) = brighter
	sig = LPF.ar(sig, (freq * 3).clip(200, 15000));

	Out.ar(0, Pan2.ar(sig * lifeEnv * amp, pan));
}).add;

// 2. State Management
~activeSynths = IdentityDictionary.new;
~lastRSSI = IdentityDictionary.new;
~scale = Scale.majorPentatonic;

OSCdef(\bleToSound, {
	arg msg;
	var mac = msg[1];
	var name = msg[2];
	var normRSSI = msg[4]; // 0.0 to 1.0
	var prevRSSI = ~lastRSSI[mac] ? normRSSI;
	var delta = (normRSSI - prevRSSI).abs;
	var groupDelta, prominence, freq, amp;

	// Update history
	~lastRSSI[mac] = normRSSI;

	if(normRSSI > 0.1, {
		// Calculate Group Movement: average delta of all known devices
		groupDelta = ~lastRSSI.values.collect({|v, i| (v - (~lastRSSI.values[i-1] ? v)).abs }).mean;

		// Map RSSI to Frequency: Inverse mapping
		// High strength (1.0) -> Low freq (degree 0)
		// Low strength (0.1) -> High freq (degree 24+)
		freq = ~scale.degreeToFreq((1 - normRSSI) * 24, 40.midicps, 1);

		// Calculate Amplitude (The "Movement" Penalty)
		// If delta is 0, volume drops to 10%. If moving with group, it boosts.
		amp = normRSSI.pow(1.5);
		amp = amp * (delta * 10).clip(0.1, 1.0); // Static penalty

		if(delta > groupDelta && delta > 0.01, {
			amp = amp * 1.5; // Group prominence boost
		});

		// Manage Synth Lifecycle
		if(~activeSynths[mac].isNil, {
			~activeSynths[mac] = Synth(\bleDynamic, [\freq, freq, \amp, amp, \sustain, 10]);
		}, {
			// Update existing synth
			~activeSynths[mac].set(\freq, freq, \amp, amp);
		});

		// Cleanup local reference if 10s passed (server handles the actual freeing)
		SystemClock.sched(10, { ~activeSynths[mac] = nil; nil });

		postf("Device: % | Delta: % | Amp: %\n", name, delta.round(0.001), amp.round(0.001));
	});
}, '/ble/data');
)
