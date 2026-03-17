(
// 1. Warm, gliding Synth
SynthDef(\bleSmooth, {
	arg freq=110, amp=0, gate=1;
	var sig, env;

	// Lagging freq and amp here stops the glitching/clicks
	var f = freq.lag(0.4);
	var a = amp.lag(0.6);

	sig = SinOsc.ar([f, f*1.005]) * 0.2;
	sig = LPF.ar(sig, f * 2);
	env = EnvGen.kr(Env.asr(1, 1, 2), gate, doneAction: 2);

	Out.ar(0, sig * env * a);
}).add;

~synths = Dictionary.new;
~scale = Scale.minorPentatonic;
~root = 36.midicps; // Low C (one octave down)

// 2. The Listener
OSCdef(\bleToSound, {
	arg msg;
	var id = msg[1].asSymbol;
	var norm = msg[3].asFloat;
	var freq = ~scale.degreeToFreq((norm * 12).round, ~root, 1);

	// If we haven't seen this MAC, make a synth
	if(~synths[id].isNil, {
		~synths[id] = Synth(\bleSmooth, [\freq, freq, \amp, 0]);
		("✨ New: " + id).postln;
	}, {
		// Update existing synth
		~synths[id].set(\freq, freq, \amp, norm.pow(2));
	});
}, '/ble/data');
)
