(
/* --- FLAVOR 3: PERCUSSIVE --- */
/* Sharp, rhythmic sounds with quick attacks and short decays */

/* --- 1. USER CONFIGURATION --- */
~cfg = (
	masterVol: 0.6,             // Punchy volume
	rootNote: 48,               // Mid range (C3)
	scale: Scale.minorPentatonic,
	minRSSI: 0.15,
	movementThreshold: 0.05,    // Higher threshold for cleaner, intentional triggering

	// DEVICE ROLES
	deviceMap: (
		'00:1a:7d:da:71:13': (type: \kick,     octave: -1),
		'a4:c1:38:8d:12:44': (type: \snare,    octave: 1),
		'default':           (type: \hihat,    octave: 2)
	)
);

/* --- 2. PERCUSSIVE INSTRUMENTS --- */

// Kick drum
SynthDef(\kick, {
	arg freq=60, amp=0, pan=0;
	var sig, env, pitchEnv;
	env = EnvGen.kr(Env.perc(0.001, 0.3), doneAction: 2);
	pitchEnv = EnvGen.ar(Env.perc(0.001, 0.05)); // Pitch drop
	sig = SinOsc.ar(freq * (1 + (pitchEnv * 2))) * env * amp;
	sig = LPF.ar(sig, 150);
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Snare with noise
SynthDef(\snare, {
	arg freq=200, amp=0, pan=0;
	var sig, env, tone, noise;
	env = EnvGen.kr(Env.perc(0.001, 0.15), doneAction: 2);
	tone = SinOsc.ar([freq, freq * 1.5]) * [0.6, 0.4];
	noise = WhiteNoise.ar(0.4) * EnvGen.ar(Env.perc(0.001, 0.08));
	sig = (tone.sum + noise) * env * amp;
	sig = HPF.ar(sig, 150);
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Hi-hat (metallic cymbal)
SynthDef(\hihat, {
	arg freq=8000, amp=0, pan=0;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.001, 0.08), doneAction: 2);
	sig = HPF.ar(WhiteNoise.ar(0.5) * env * amp, 6000);
	sig = BPF.ar(sig, freq, 0.3);
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Minimal Reverb (drier for percussive clarity)
SynthDef(\globalReverb, {
	arg out=0, mix=0.15; // Less reverb
	var input = In.ar(~reverbBus, 2);
	var sig = FreeVerb.ar(input, mix, 0.5, 0.3); // Smaller room
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
		// Rate limiting: max 1 strike every 0.2 seconds (faster for percussion)
		if((now - (~lastStrike[mac] ? 0)) > 0.2, {
			degree = (1 - normRSSI) * 15;
			freq = ~cfg.scale.degreeToFreq(degree, (~cfg.rootNote + (devCfg.octave * 12)).midicps, 1);
			amp = (normRSSI.pow(1.0) * (delta * 12)).clip(0.2, 1.0); // Punchier dynamics

			// Trigger a discrete note
			Synth(devCfg.type, [
				\freq, freq,
				\amp, amp,
				\pan, rrand(-0.5, 0.5)
			]);

			~lastStrike[mac] = now;
			postf(">> PERC: % [%] | RSSI: % | Amp: %\n", name, devCfg.type, normRSSI.round(0.01), amp.round(0.01));
		});
	});
}, '/ble/data');
)
