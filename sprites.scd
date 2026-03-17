(
s.boot; // 1. Make sure the server is green in the bottom right!

/* --- 1. USER CONFIGURATION --- */
~cfg = (
    masterVol: 0.6,
    rootNote: 36,
    scale: Scale.minorPentatonic,
    movementThreshold: 0.02,    // Lowered to make it easier to trigger

    deviceMap: (
        'default': (type: \woodChime, octave: 0)
    )
);

/* --- 2. SIMPLIFIED INSTRUMENT (Direct to Output) --- */
SynthDef(\woodChime, {
    arg freq=220, amp=0, pan=0;
    var sig, env;
    // Percussive envelope: short attack, medium decay
    env = EnvGen.kr(Env.perc(0.002, 1.0), doneAction: 2);
    // Simple resonant wood-like tone
    sig = Ringz.ar(PinkNoise.ar(0.1) * EnvGen.ar(Env.perc(0.001, 0.01)), [freq, freq*2.01], 0.5).sum;
    sig = LPF.ar(sig, 3000) * env * amp * ~cfg.masterVol;
    Out.ar(0, Pan2.ar(sig, pan));
}).add;

/* --- 3. SYSTEM STATE --- */
~lastRSSI = IdentityDictionary.new;
~lastStrike = IdentityDictionary.new;

/* --- 4. DATA CONDUCTOR --- */
OSCdef(\bleToSound, {
    arg msg;
    // INDEX CORRECTION:
    // msg[0] = "/ble/data"
    // msg[1] = MAC, msg[2] = Name, msg[4] = RSSI (based on your first script)
    var mac      = msg[1].asSymbol;
    var name     = msg[2];
    var normRSSI = msg[4].asFloat;

    var prevRSSI = ~lastRSSI[mac] ? normRSSI;
    var delta    = (normRSSI - prevRSSI).abs;
    var now      = Main.elapsedTime;

    ~lastRSSI[mac] = normRSSI;

    // Log EVERYTHING to see if we hit the logic
    // postf("Data: % | RSSI: % | Delta: %\n", name, normRSSI.round(0.02), delta.round(0.02));

    if(delta > ~cfg.movementThreshold) {
        if((now - (~lastStrike[mac] ? 0)) > 0.2) {

            var degree = (1 - normRSSI) * 12;
            var freq = ~cfg.scale.degreeToFreq(degree, ~cfg.rootNote.midicps, 1);
            var amp = (normRSSI.pow(1.2) * (delta * 15)).clip(0.1, 1.0);

            // Direct Trigger
            Synth(\woodChime, [\freq, freq, \amp, amp, \pan, rrand(-0.5, 0.5)]);

            ~lastStrike[mac] = now;
            postf(">> TRIGGER: % | Freq: % | Amp: %\n", name, freq.round, amp.round(0.1));
        };
    };
}, '/ble/data');
)
