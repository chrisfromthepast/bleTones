(
// 1. Setup the Synth (The Instrument)
SynthDef(\bleDrone, {
	arg freq=440, amp=0, gate=1, pan=0;
	var sig, env;
	// A soft, shimmering sawtooth through a low-pass filter
	sig = Saw.ar([freq, freq * 1.01]) * 0.2;
	sig = LPF.ar(sig, freq * 2);
	env = EnvGen.kr(Env.asr(0.1, 1, 1), gate, doneAction: 2);
	Out.ar(0, Pan2.ar(sig * env * amp, pan));
}).add;

// 2. Variables for mapping
~scale = Scale.majorPentatonic; // Change to .minorPentatonic for a moody vibe
~activeSynths = IdentityDictionary.new;

// 3. The OSC Receiver
OSCdef(\bleToSound, {
	arg msg;
	var mac = msg[1];
	var name = msg[2];
	var normRSSI = msg[4]; // 0.0 to 1.0
	var degree, freq;

	// Only make sound if the signal is strong enough (norm > 0.2)
	if(normRSSI > 0.1, {

		// Map 0.0-1.0 to a scale degree (0 to 12 notes)
		degree = (normRSSI * 12).round;
		freq = ~scale.degreeToFreq(degree, 60.midicps, 1); // Middle C root

		// If this is a new device, start a synth for it
		if(~activeSynths[mac].isNil, {
			~activeSynths[mac] = Synth(\bleDrone, [\freq, freq, \amp, 0]);
		});

		// Update the synth's frequency and volume
		~activeSynths[mac].set(\freq, freq, \amp, normRSSI.pow(2)); // pow(2) makes volume curve smoother

		postf("Device: % | Note Freq: % Hz\n", name, freq);
	});
}, '/ble/data');
)
