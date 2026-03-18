(
s.boot;

/* --- 1. USER CONFIGURATION --- */
~cfg = (
    masterVol: 0.6,
    rootNote: 36,
    scale: Scale.minorPentatonic,
    tempo: 110 / 60,            // 110 BPM
    movementThreshold: 0.025,
    fadeSpeed: 0.15,            // How fast it fades to silence (higher = faster)

    instList: [\deepLog, \woodChime, \hollowReed]
);

/* --- 2. CLEAN INSTRUMENTS (Direct Output) --- */

SynthDef(\woodChime, { |freq=220, amp=0, pan=0|
    var sig, env = EnvGen.kr(Env.perc(0.002, 0.8), doneAction: 2);
    sig = Ringz.ar(PinkNoise.ar(0.05) * EnvGen.ar(Env.perc(0.001, 0.01)), [freq, freq*2.01], 0.4).sum;
    sig = LPF.ar(sig, 2500) * env * amp * ~cfg.masterVol;
    Out.ar(0, Pan2.ar(sig, pan));
}).add;

SynthDef(\deepLog, { |freq=110, amp=0, pan=0|
    var sig, env = EnvGen.kr(Env.perc(0.01, 1.2), doneAction: 2);
    sig = Ringz.ar(BrownNoise.ar(0.1) * EnvGen.ar(Env.perc(0.005, 0.01)), [freq, freq*1.5], 0.8).sum;
    sig = LPF.ar(sig, 1000) * env * amp * ~cfg.masterVol;
    Out.ar(0, Pan2.ar(sig, pan));
}).add;

SynthDef(\hollowReed, { |freq=440, amp=0, pan=0|
    var sig, env = EnvGen.kr(Env.perc(0.002, 0.5), doneAction: 2);
    sig = Ringz.ar(WhiteNoise.ar(0.03) * EnvGen.ar(Env.perc(0.001, 0.005)), [freq*2, freq*3], 0.3).sum;
    sig = BPF.ar(sig, freq*2, 0.4) * env * amp * ~cfg.masterVol;
    Out.ar(0, Pan2.ar(sig, pan));
}).add;

/* --- 3. THE MUSICAL BRAIN --- */
~lastRSSI = IdentityDictionary.new;
~globalAmp = 0; // The "Song Volume" that fades in/out

// A background process to handle the "Fade to Silence"
Tdef(\fader, {
    loop {
        ~globalAmp = (~globalAmp - 0.02).max(0); // Slowly die
        0.1.wait;
    }
}).play;

OSCdef(\bleToSound, {
    arg msg;
    var mac      = msg[1].asSymbol;
    var name     = msg[2];
    var normRSSI = msg[4].asFloat;

    var prevRSSI = ~lastRSSI[mac] ? normRSSI;
    var delta    = (normRSSI - prevRSSI).abs;

    var instIndex = mac.hash % ~cfg.instList.size;
    var instrument = ~cfg.instList[instIndex];

    ~lastRSSI[mac] = normRSSI;

    if(delta > ~cfg.movementThreshold) {
        // Boost the global song volume when movement happens
        ~globalAmp = (~globalAmp + 0.15).min(1.0);

        // QUANTIZE: Schedule the note for the next "grid" 8th note
        s.makeBundle(s.latency, {
            var degree = (1 - normRSSI) * 12;
            var freq = ~cfg.scale.degreeToFreq(degree, ~cfg.rootNote.midicps, 1);
            var noteAmp = (normRSSI.pow(1.2) * (delta * 15)).clip(0.1, 1.0) * ~globalAmp;

            Synth(instrument, [\freq, freq, \amp, noteAmp, \pan, rrand(-0.4, 0.4)]);
        });

        postf(">> % [%] Playing: % (Vol: %)\n", name, mac, instrument, ~globalAmp.round(0.1));
    };
}, '/ble/data');
)
