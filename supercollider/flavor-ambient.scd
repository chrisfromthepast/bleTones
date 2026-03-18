(
/* --- FLAVOR 2: AMBIENT --- */
/* Spacious, atmospheric tones with long sustain and reverb */

/* --- 1. USER CONFIGURATION --- */
~cfg = (
	masterVol: 0.35,            // Lower volume for more subtle atmosphere
	rootNote: 36,               // Deep bass for warmth (C2)
	scale: Scale.majorPentatonic, // Major for brighter, uplifting feel
	minRSSI: 0.15,
	movementThreshold: 0.03,    // Lower threshold for more sensitivity

	// DEVICE ROLES
	deviceMap: (
		'00:1a:7d:da:71:13': (type: \pad,      octave: 0),
		'a4:c1:38:8d:12:44': (type: \shimmer,  octave: 3),
		'default':           (type: \bell,     octave: 2)
	)
);

/* --- 2. AMBIENT INSTRUMENTS --- */

// Deep pad with long sustain
SynthDef(\pad, {
	arg freq=100, amp=0, pan=0;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.1, 3.0, 1, -2), doneAction: 2); // Long attack and decay
	sig = SinOsc.ar(freq * [1, 1.01, 2.0, 2.02]) * [1, 0.8, 0.3, 0.2];
	sig = sig.sum * env * amp;
	sig = LPF.ar(sig, freq * 6); // Warmer tone
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Bright shimmer with harmonics
SynthDef(\shimmer, {
	arg freq=800, amp=0, pan=0;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.01, 2.5, 1, -3), doneAction: 2);
	sig = SinOsc.ar(freq * [1, 2, 3, 4, 5]) * [1, 0.5, 0.3, 0.2, 0.1];
	sig = sig.sum * env * amp * 0.6;
	sig = HPF.ar(sig, freq * 0.8); // Brighter, airier
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Bell-like tone
SynthDef(\bell, {
	arg freq=440, amp=0, pan=0;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.001, 2.0, 1, -4), doneAction: 2);
	sig = SinOsc.ar(freq * [1, 2.01, 3.01, 4.03, 5.02]) * [1, 0.6, 0.4, 0.2, 0.1];
	sig = sig.sum * env * amp;
	sig = LPF.ar(sig, freq * 8);
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Global Reverb with MORE space
SynthDef(\globalReverb, {
	arg out=0, mix=0.5; // More reverb mix
	var input = In.ar(~reverbBus, 2);
	var sig = FreeVerb.ar(input, mix, 0.9, 0.8); // Larger room
	Out.ar(out, sig * ~cfg.masterVol);
}).add;

/* --- 3. SYSTEM BOOT --- */
~reverbBus = Bus.audio(s, 2);
s.makeBundle(nil, { ~reverb = Synth.tail(nil, \globalReverb) });

~lastRSSI = IdentityDictionary.new;
~lastStrike = IdentityDictionary.new;

OSCdef(\bleToSound, {
	arg msg;
	var mac = msg[1].asSymbol, name = msg[2], normRSSI = msg[4].asFloat;
	var prevRSSI = ~lastRSSI[mac] ? normRSSI;
	var delta = (normRSSI - prevRSSI).abs;
	var devCfg = ~cfg.deviceMap[mac] ? ~cfg.deviceMap['default'];
	var now = Main.elapsedTime;
	var freq, amp, degree;

	~lastRSSI[mac] = normRSSI;

	// ONLY STRIKE IF MOVING
	if(delta > ~cfg.movementThreshold, {
		// Rate limiting: max 1 strike every 0.5 seconds per device (slower for ambient)
		if((now - (~lastStrike[mac] ? 0)) > 0.5, {
			degree = (1 - normRSSI) * 15;
			freq = ~cfg.scale.degreeToFreq(degree, (~cfg.rootNote + (devCfg.octave * 12)).midicps, 1);
			amp = (normRSSI.pow(1.5) * (delta * 8)).clip(0.08, 0.8); // Softer dynamics

			// Trigger a discrete note
			Synth(devCfg.type, [
				\freq, freq,
				\amp, amp,
				\pan, rrand(-0.9, 0.9) // Wider stereo field
			]);

			~lastStrike[mac] = now;
			postf(">> AMBIENT: % [%] | RSSI: % | Amp: %\n", name, devCfg.type, normRSSI.round(0.01), amp.round(0.01));
		});
	});
}, '/ble/data');
)
