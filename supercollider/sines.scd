(
// 1. Physical Modeling Synth (Wooden Chimes)
SynthDef(\woodChime, {
	arg freq=220, amp=0, pan=0, sustain=10;
	var exciter, sig, lifeEnv;

	// Self-destruct after 10 seconds
	lifeEnv = EnvGen.kr(Env.linen(0.01, sustain - 0.1, 0.1), doneAction: 2);

	// Exciter: A tiny click/burst of noise simulates the "strike"
	exciter = PinkNoise.ar(0.1) * EnvGen.ar(Env.perc(0.001, 0.01));

	// Resonator: Ringz turns the click into a pitched tone
	// Using multiple Ringz creates the "complex" harmonics of wood
	sig = Ringz.ar(exciter, [freq, freq * 2.02, freq * 3.15], [0.5, 0.3, 0.1]).sum;

	// Dampen high frequencies for a warmer, "thuddy" wood feel
	sig = LPF.ar(sig, freq * 4);

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
	var normRSSI = msg[4];
	var prevRSSI = ~lastRSSI[mac] ? normRSSI;
	var delta = (normRSSI - prevRSSI).abs;
	var groupDelta, freq, amp;

	~lastRSSI[mac] = normRSSI;

	if(normRSSI > 0.1, {
		// Calculate movement across the group
		groupDelta = ~lastRSSI.values.collect({|v, i| (v - (~lastRSSI.values[i-1] ? v)).abs }).mean;

		// OCTAVE SHIFT: 36.midicps instead of 48 or 60 drops it significantly
		// Near (High RSSI) = Low, Deep Wood | Far (Low RSSI) = High, Twinkly Chimes
		freq = ~scale.degreeToFreq((1 - normRSSI) * 15, 36.midicps, 1);

		// Movement-based Volume
		amp = normRSSI.pow(1.2);
		amp = amp * (delta * 15).clip(0.05, 1.0); // Extreme penalty for static objects

		// Group Boost
		if(delta > groupDelta && delta > 0.01, { amp = amp * 2 });

		// Lifecycle Management
		if(~activeSynths[mac].isNil, {
			~activeSynths[mac] = Synth(\woodChime, [\freq, freq, \amp, amp, \sustain, 10]);
		}, {
			~activeSynths[mac].set(\freq, freq, \amp, amp);
		});

		SystemClock.sched(10, { ~activeSynths[mac] = nil; nil });
	});
}, '/ble/data');
)
