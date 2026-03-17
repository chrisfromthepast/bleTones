(
/* --- FLAVOR 4: ETHEREAL --- */
/* High frequencies, long sustain, dreamy atmosphere */

/* --- 1. USER CONFIGURATION --- */
~cfg = (
	masterVol: 0.3,             // Softer for delicate sounds
	rootNote: 60,               // High range (C4)
	scale: Scale.lydian,        // Lydian mode for dreamy, floating feel
	minRSSI: 0.15,
	movementThreshold: 0.025,   // Very sensitive

	// DEVICE ROLES
	deviceMap: (
		'00:1a:7d:da:71:13': (type: \crystal,  octave: 0),
		'a4:c1:38:8d:12:44': (type: \glass,    octave: 1),
		'default':           (type: \wind,     octave: 2)
	)
);

/* --- 2. ETHEREAL INSTRUMENTS --- */

// Crystal resonance
SynthDef(\crystal, {
	arg freq=440, amp=0, pan=0;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.01, 4.0, 1, -3), doneAction: 2); // Very long decay
	sig = SinOsc.ar(freq * [1, 2.01, 3.03, 4.05, 5.07, 6.09]) * [1, 0.6, 0.4, 0.3, 0.2, 0.1];
	sig = sig.sum * env * amp * 0.7;
	sig = HPF.ar(sig, freq * 0.5); // Keep it bright
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Glass harmonica
SynthDef(\glass, {
	arg freq=660, amp=0, pan=0;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.02, 3.5, 1, -4), doneAction: 2);
	sig = SinOsc.ar(freq * [1, 1.99, 3.01, 4.97]) * [1, 0.5, 0.3, 0.15];
	sig = sig.sum * env * amp;
	sig = BPF.ar(sig, freq * 2.5, 0.5); // Focused resonance
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Wind chimes (airy, delicate)
SynthDef(\wind, {
	arg freq=880, amp=0, pan=0;
	var sig, env, exciter;
	env = EnvGen.kr(Env.perc(0.005, 5.0, 1, -4), doneAction: 2); // Very long
	exciter = PinkNoise.ar(0.1) * EnvGen.ar(Env.perc(0.001, 0.02));
	sig = Ringz.ar(exciter, freq * [1, 1.5, 2.3, 3.1, 4.2], [2, 1.5, 1.2, 1.0, 0.8]) * [1, 0.6, 0.4, 0.3, 0.2];
	sig = sig.sum * env * amp;
	sig = HPF.ar(sig, freq * 0.8); // Keep it light and airy
	Out.ar(~reverbBus, Pan2.ar(sig, pan));
}).add;

// Global Reverb with MAXIMUM space
SynthDef(\globalReverb, {
	arg out=0, mix=0.6; // Heavy reverb
	var input = In.ar(~reverbBus, 2);
	var sig = FreeVerb.ar(input, mix, 0.95, 0.9); // Huge room
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
		// Rate limiting: max 1 strike every 0.8 seconds (slower, more spacious)
		if((now - (~lastStrike[mac] ? 0)) > 0.8, {
			degree = (1 - normRSSI) * 12; // Narrower range for more consonance
			freq = ~cfg.scale.degreeToFreq(degree, (~cfg.rootNote + (devCfg.octave * 12)).midicps, 1);
			amp = (normRSSI.pow(1.8) * (delta * 6)).clip(0.05, 0.7); // Gentle dynamics

			// Trigger a discrete note
			Synth(devCfg.type, [
				\freq, freq,
				\amp, amp,
				\pan, rrand(-1.0, 1.0) // Full stereo width
			]);

			~lastStrike[mac] = now;
			postf(">> ETHEREAL: % [%] | RSSI: % | Amp: %\n", name, devCfg.type, normRSSI.round(0.01), amp.round(0.01));
		});
	});
}, '/ble/data');
)
