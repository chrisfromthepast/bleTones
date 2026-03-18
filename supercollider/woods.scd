(
/* --- FLAVOR 1: MELODIC --- */
/* Balanced, musical tones with moderate sustain and wooden character */

/* --- 1. USER CONFIGURATION --- */
~cfg = (
	masterVol: 0.5,             // Overall output level (0.0 to 1.0)
	rootNote: 48,               // Middle range (C3)
	scale: Scale.minorPentatonic,
	minRSSI: 0.15,
	movementThreshold: 0.04,    // RSSI must change by this much to "strike"

	// DEVICE ROLES: Assign types to MACs
	// Types: \logDrum (Deep), \claves (Sharp), \kalimba (Metallic Wood)
	deviceMap: (
		'00:1a:7d:da:71:13': (type: \logDrum,  octave: 0),
		'a4:c1:38:8d:12:44': (type: \claves,   octave: 2),
		'default':           (type: \kalimba,  octave: 1)
	)
);

/* --- 2. THE INSTRUMENTS (PERCUSSIVE ONLY) --- */

// Deep, hollow log drum
SynthDef(\logDrum, {
	arg freq=100, amp=0, pan=0;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.005, 0.8), doneAction: 2);
	sig = SinOsc.ar(freq * [1, 1.5, 2.6]) * [1, 0.4, 0.2];
	sig = sig.sum * env * amp;
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// High, dry wooden claves
SynthDef(\claves, {
	arg freq=800, amp=0, pan=0;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.001, 0.1), doneAction: 2);
	sig = Formant.ar(freq, freq * 1.5, 200) * env * amp;
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// "Wooden" Kalimba (thumb piano)
SynthDef(\kalimba, {
	arg freq=440, amp=0, pan=0;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.002, 1.2, 1, -4), doneAction: 2);
	sig = SinOsc.ar(freq) + (Saw.ar(freq * 2.01, 0.1));
	sig = LPF.ar(sig, freq * 3) * env * amp;
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Global Reverb for "Space"
SynthDef(\globalReverb, {
	arg out=0, mix=0.3;
	var input = In.ar(~reverbBus, 2);
	var sig = FreeVerb.ar(input, mix, 0.8, 0.5);
	Out.ar(out, sig * ~cfg.masterVol);
}).add;

/* --- 3. SYSTEM BOOT --- */
~reverbBus = Bus.audio(s, 2);
s.makeBundle(nil, { ~reverb = Synth.tail(nil, \globalReverb) });

~lastRSSI = IdentityDictionary.new;
~lastStrike = IdentityDictionary.new; // To prevent machine-gunning notes

OSCdef(\bleToSound, {
	arg msg;
	var mac = msg[1].asSymbol, name = msg[2], normRSSI = msg[4].asFloat;
	var prevRSSI = ~lastRSSI[mac] ? normRSSI;
	var delta = (normRSSI - prevRSSI).abs;
	var devCfg = ~cfg.deviceMap[mac] ? ~cfg.deviceMap['default'];
	var now = Main.elapsedTime;
	var freq, amp, degree;

	~lastRSSI[mac] = normRSSI;

	// ONLY STRIKE IF MOVING (Removes Drones)
	if(delta > ~cfg.movementThreshold, {

		// Rate limiting: max 1 strike every 0.3 seconds per device
		if((now - (~lastStrike[mac] ? 0)) > 0.3, {

			degree = (1 - normRSSI) * 15;
			freq = ~cfg.scale.degreeToFreq(degree, (~cfg.rootNote + (devCfg.octave * 12)).midicps, 1);
			amp = (normRSSI.pow(1.2) * (delta * 10)).clip(0.1, 1.0);

			// Trigger a discrete note
			Synth(devCfg.type, [
				\freq, freq,
				\amp, amp,
				\pan, rrand(-0.7, 0.7)
			]);

			~lastStrike[mac] = now;
			postf(">> STRIKE: % [%] | RSSI: % | Amp: %\n", name, devCfg.type, normRSSI.round(0.01), amp.round(0.01));
		});
	});
}, '/ble/data');
)
