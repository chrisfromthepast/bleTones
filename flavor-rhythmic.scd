(
/* --- FLAVOR 5: RHYTHMIC --- */
/* Fast-paced, energetic sounds with tight timing and groove */

/* --- 1. USER CONFIGURATION --- */
~cfg = (
	masterVol: 0.55,            // Good presence
	rootNote: 40,               // Low-mid range (E2)
	scale: Scale.minor,         // Full minor scale for more note choices
	minRSSI: 0.15,
	movementThreshold: 0.06,    // Higher threshold for intentional strikes

	// DEVICE ROLES
	deviceMap: (
		'00:1a:7d:da:71:13': (type: \bass,     octave: 0),
		'a4:c1:38:8d:12:44': (type: \pluck,    octave: 1),
		'default':           (type: \marimba,  octave: 1)
	)
);

/* --- 2. RHYTHMIC INSTRUMENTS --- */

// Bass note (short and punchy)
SynthDef(\bass, {
	arg freq=80, amp=0, pan=0;
	var sig, env, pitchEnv;
	env = EnvGen.kr(Env.perc(0.002, 0.25), doneAction: 2);
	pitchEnv = EnvGen.ar(Env.perc(0.001, 0.03));
	sig = SinOsc.ar(freq * (1 + pitchEnv)) * env * amp;
	sig = sig + (LPF.ar(WhiteNoise.ar(0.1), 200) * EnvGen.ar(Env.perc(0.001, 0.02)) * amp);
	sig = LPF.ar(sig, 200);
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Plucked string
SynthDef(\pluck, {
	arg freq=220, amp=0, pan=0;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.001, 0.4, 1, -5), doneAction: 2); // Fast decay
	sig = Pluck.ar(WhiteNoise.ar(0.1), 1, 0.2, freq.reciprocal, 10, 0.5);
	sig = sig * env * amp;
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Marimba (pitched percussion)
SynthDef(\marimba, {
	arg freq=440, amp=0, pan=0;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.001, 0.6, 1, -6), doneAction: 2);
	sig = SinOsc.ar(freq * [1, 2.76, 5.40]) * [1, 0.3, 0.15]; // Marimba-like harmonics
	sig = sig.sum * env * amp;
	sig = LPF.ar(sig, freq * 4);
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Tight reverb for rhythmic clarity
SynthDef(\globalReverb, {
	arg out=0, mix=0.2; // Minimal reverb
	var input = In.ar(~reverbBus, 2);
	var sig = FreeVerb.ar(input, mix, 0.6, 0.4); // Small, tight space
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
		// Rate limiting: max 1 strike every 0.15 seconds (fast for rhythmic playing)
		if((now - (~lastStrike[mac] ? 0)) > 0.15, {
			degree = (1 - normRSSI) * 20; // Wider range for more variety
			freq = ~cfg.scale.degreeToFreq(degree, (~cfg.rootNote + (devCfg.octave * 12)).midicps, 1);
			amp = (normRSSI.pow(0.9) * (delta * 14)).clip(0.15, 1.0); // Dynamic range

			// Trigger a discrete note
			Synth(devCfg.type, [
				\freq, freq,
				\amp, amp,
				\pan, rrand(-0.6, 0.6)
			]);

			~lastStrike[mac] = now;
			postf(">> RHYTHM: % [%] | RSSI: % | Amp: %\n", name, devCfg.type, normRSSI.round(0.01), amp.round(0.01));
		});
	});
}, '/ble/data');
)
