(
/* --- 1. USER CONFIGURATION --- */
~cfg = (
	rootNote: 36,
	scale: Scale.minorPentatonic,
	maxDuration: 4,
	minRSSI: 0.12,
	staticPenalty: 50,          // Higher = silence static objects faster

	// DEVICE ROLES: Assign a 'type' to specific MACs
	// Types: \marimba (Deep), \woodBlock (High/Dry), \bamboo (Smooth)
	deviceMap: (
		'00:1a:7d:da:71:13': (type: \marimba,   octave: 0),
		'a4:c1:38:8d:12:44': (type: \woodBlock, octave: 1),
		'default':           (type: \bamboo,    octave: 0)
	)
);

/* --- 2. THE WOODEN ORCHESTRA --- */

// A deep, resonant marimba-style strike
SynthDef(\marimba, {
	arg freq=220, amp=0, sustain=8;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.001, sustain*0.5), doneAction: 2);
	sig = BPZ2.ar(WhiteNoise.ar(0.1)) * EnvGen.ar(Env.perc(0.001, 0.01)); // strike
	sig = Ringz.ar(sig, [freq, freq*2.01], [0.4, 0.2]).sum;
	sig = sig * env * amp * 2;
	Out.ar(0, Pan2.ar(sig, 0));
}).add;

// A short, dry "clack"
SynthDef(\woodBlock, {
	arg freq=220, amp=0, sustain=8;
	var sig, env;
	env = EnvGen.kr(Env.perc(0.001, 0.1), doneAction: 2);
	sig = Ringz.ar(PinkNoise.ar(0.1), [freq*3, freq*4.2], [0.05, 0.02]).sum;
	sig = sig * env * amp * 3;
	Out.ar(0, Pan2.ar(sig, 0.2));
}).add;

// A breathy, sustained wooden flute/pipe
SynthDef(\bamboo, {
	arg freq=220, amp=0, sustain=8;
	var sig, env, noise;
	env = EnvGen.kr(Env.linen(0.2, sustain-0.4, 0.2), doneAction: 2);
	noise = WhiteNoise.ar(0.03) * LPF.ar(Resonz.ar(WhiteNoise.ar, freq, 0.001), 2000);
	sig = SinOsc.ar(freq + (noise * 100)) * 0.2; // Add "breath" jitter
	sig = sig + noise;
	sig = LPF.ar(sig, freq * 2);
	Out.ar(0, Pan2.ar(sig * env * amp, -0.2));
}).add;

/* --- 3. THE CONDUCTOR (OSC Logic) --- */
~activeSynths = IdentityDictionary.new;
~lastRSSI = IdentityDictionary.new;

OSCdef(\bleToSound, {
	arg msg;
	var mac = msg[1].asSymbol, name = msg[2], normRSSI = msg[4].asFloat;
	var prevRSSI = ~lastRSSI[mac] ? normRSSI;
	var delta = (normRSSI - prevRSSI).abs;
	var devCfg = ~cfg.deviceMap[mac] ? ~cfg.deviceMap['default'];
	var freq, amp, degree;

	~lastRSSI[mac] = normRSSI;

	if(normRSSI > ~cfg.minRSSI, {
		// PITCH: RSSI maps to scale degree
		degree = (1 - normRSSI) * 12;
		freq = ~cfg.scale.degreeToFreq(degree, (~cfg.rootNote + (devCfg.octave * 12)).midicps, 1);

		// VOLUME: Strongly biased toward movement
		amp = (normRSSI.pow(1.5) * (delta * ~cfg.staticPenalty)).clip(0, 1.2);

		if(amp > 0.02, {
			postf(">> % [%] | Tone: % | Amp: %\n", name, mac, devCfg.type, amp.round(0.01));

			// If no synth or if the synth is old (stopped making sound), trigger new strike
			if(~activeSynths[mac].isNil, {
				~activeSynths[mac] = Synth(devCfg.type, [
					\freq, freq, \amp, amp, \sustain, ~cfg.maxDuration
				]);
				SystemClock.sched(~cfg.maxDuration, { ~activeSynths[mac] = nil; nil });
			}, {
				// For \bamboo (sustained), we update freq/amp.
				// For strikes (\marimba), we only update amp or re-trigger if delta is huge.
				if(devCfg.type == \bamboo, {
					~activeSynths[mac].set(\freq, freq, \amp, amp);
				}, {
					if(delta > 0.05, { // Re-strike on significant movement
						~activeSynths[mac].set(\amp, amp, \freq, freq);
					});
				});
			});
		});
	});
}, '/ble/data');
)
