#include "CarCatalog.hpp"

// Les valeurs sont des approximations a partir des fiches techniques, pas des mesures.
// Rapports de boite : valeurs reelles, la voiture les multiplie par le rapport de pont.

namespace {

// Inertie en lacet et raideur de pneu deduites de la masse, calees sur la WRX (2500 kg*m^2 et 120000 N/rad pour 1530 kg).
void deriveChassis(CarSpec& s, float frontWeightFrac, float tireStiffnessFactor) {
    s.frontWeightFrac = frontWeightFrac;
    s.yawInertia = 0.229f * s.weight * s.wheelBase * s.wheelBase;
    s.cornerStiffness = 78.4f * s.weight * tireStiffnessFactor;
    s.bodyCenterFromRearAxle = 0.47f * s.wheelBase;
}

CarModel subaruWrx() {
    CarModel m;
    m.name = "Subaru WRX";
    m.sprite = "Cars/WRX.png";
    // CarSpec, Engine::Config et EngineSynth::Config par defaut = WRX (FA24 2.4 L turbo, boxer 4 cylindres)
    return m;
}

CarModel twingo() {
    CarModel m;
    m.name = "Renault Twingo I";
    m.sprite = "Cars/Twingo.png";

    CarSpec& s = m.spec;
    s.size = {3.43f, 1.63f};
    s.wheelBase = 2.347f;
    s.weight = 830.0f;
    s.wheelDiameter = 0.547f;                  // 155/70 R13
    s.gearRatios = {-3.545f, 0.0f, 3.727f, 2.048f, 1.321f, 0.967f, 0.756f, 0.0f, 0.0f};
    s.maxGear = 5;
    s.finalDrive = 3.733f;                     // boite JB1-962 (a confirmer pour la D7F)
    s.driveFrontShare = 1.0f;                  // traction
    s.clutchMaxTorque = 200.0f;
    s.maxSteeringAngle = 35.0f * DEG2RAD;
    s.steerSpeedRef = 16.0f;
    s.maxBrakeForce = 7500.0f;
    s.CdA = 0.67f;
    s.tireGrip = 0.85f;
    deriveChassis(s, 0.62f, 0.9f);

    // D7F 1.2 L 8 soupapes : 44 kW a 5250 tr/min, 93 Nm a 2500 tr/min
    Engine::Config& e = m.engine;
    e.idleRPM = 850.0f;
    e.maxRPM = 6000.0f;
    e.maxTorque = 93.0f;
    e.inertia = 0.10f;
    e.frictionTorque = 5.0f;
    e.frictionPerRPM = 0.003f;
    e.starterTorque = 60.0f;
    e.compressionTorque = 22.0f;
    e.torqueCurve = {{
        {0.0f, 0.55f}, {800.0f, 0.75f}, {1500.0f, 0.90f}, {2500.0f, 1.00f},
        {3500.0f, 0.99f}, {4500.0f, 0.93f}, {5250.0f, 0.82f}, {6000.0f, 0.70f}
    }};

    m.sound.cylinders = 4;
    m.sound.firingOrder = {0, 2, 3, 1, 0, 0, 0, 0};  // 4 en ligne, 1-3-4-2
    m.sound.resonanceHz = 95.0f;
    m.sound.brightness = 0.75f;                      // petit echappement, son etouffe
    m.sound.damping = 0.8f;
    return m;
}

CarModel skylineR34() {
    CarModel m;
    m.name = "Nissan Skyline GT-R R34";
    m.sprite = "Cars/Skyline.png";

    CarSpec& s = m.spec;
    s.size = {4.60f, 1.785f};
    s.wheelBase = 2.665f;
    s.weight = 1560.0f;
    s.wheelDiameter = 0.652f;                  // 245/45 R17
    s.gearRatios = {-3.438f, 0.0f, 3.827f, 2.360f, 1.685f, 1.312f, 1.000f, 0.811f, 0.0f};
    s.maxGear = 6;
    s.finalDrive = 3.545f;
    s.driveFrontShare = 0.3f;                  // ATTESA E-TS : transmission integrale a dominante arriere
    s.clutchMaxTorque = 700.0f;
    s.maxBrakeForce = 14500.0f;
    s.CdA = 0.67f;
    deriveChassis(s, 0.55f, 1.0f);

    // RB26DETT 2.6 L biturbo : 206 kW a 6800 tr/min, 392 Nm a 4400 tr/min
    Engine::Config& e = m.engine;
    e.idleRPM = 800.0f;
    e.maxRPM = 8000.0f;
    e.maxTorque = 392.0f;
    e.inertia = 0.25f;
    e.frictionTorque = 10.0f;
    e.frictionPerRPM = 0.006f;
    e.starterTorque = 100.0f;
    e.compressionTorque = 45.0f;
    e.torqueCurve = {{
        {0.0f, 0.30f}, {1000.0f, 0.40f}, {2000.0f, 0.62f}, {3000.0f, 0.92f},
        {4400.0f, 1.00f}, {5500.0f, 0.90f}, {6800.0f, 0.74f}, {8000.0f, 0.52f}
    }};

    m.sound.cylinders = 6;
    m.sound.firingOrder = {0, 4, 2, 5, 1, 3, 0, 0};  // 6 en ligne, 1-5-3-6-2-4
    m.sound.resonanceHz = 120.0f;
    m.sound.brightness = 1.0f;
    return m;
}

CarModel renegade4xe() {
    CarModel m;
    m.name = "Jeep Renegade 4xe";
    m.sprite = "Cars/Renegade.png";

    CarSpec& s = m.spec;
    s.size = {4.236f, 1.805f};
    s.wheelBase = 2.570f;
    s.weight = 1800.0f;                        // 1770 a 1850 selon les sources
    s.wheelDiameter = 0.690f;                  // 235/55 R17
    s.gearRatios = {-3.394f, 0.0f, 4.148f, 2.370f, 1.556f, 1.155f, 0.859f, 0.686f, 0.0f};
    s.maxGear = 6;
    s.finalDrive = 4.1f;
    s.driveFrontShare = 0.5f;                  // moteur thermique devant, moteur electrique derriere
    s.clutchMaxTorque = 700.0f;
    s.maxSteeringAngle = 33.0f * DEG2RAD;
    s.steerSpeedRef = 16.0f;
    s.maxBrakeForce = 15000.0f;
    s.CdA = 0.94f;
    s.tireGrip = 0.9f;
    deriveChassis(s, 0.57f, 0.95f);

    // Equivalent du groupe hybride rechargeable : 177 kW cumules a 5200 tr/min, le moteur electrique
    // apporte du couple des bas regimes. Une seule courbe moteur modelise l'ensemble.
    Engine::Config& e = m.engine;
    e.idleRPM = 800.0f;
    e.maxRPM = 6200.0f;
    e.maxTorque = 400.0f;
    e.inertia = 0.20f;
    e.frictionTorque = 9.0f;
    e.frictionPerRPM = 0.005f;
    e.starterTorque = 100.0f;
    e.compressionTorque = 35.0f;
    e.torqueCurve = {{
        {0.0f, 0.70f}, {1000.0f, 0.85f}, {1500.0f, 1.00f}, {4000.0f, 1.00f},
        {5200.0f, 0.81f}, {5800.0f, 0.72f}, {6000.0f, 0.66f}, {6200.0f, 0.60f}
    }};

    m.sound.cylinders = 4;
    m.sound.firingOrder = {0, 2, 3, 1, 0, 0, 0, 0};  // 4 en ligne, 1-3-4-2
    m.sound.resonanceHz = 100.0f;
    m.sound.brightness = 0.8f;                       // SUV : echappement filtre
    m.sound.damping = 0.9f;
    return m;
}

CarModel gt3Rs() {
    CarModel m;
    m.name = "Porsche 911 GT3 RS";
    m.sprite = "Cars/GT3RS.png";

    CarSpec& s = m.spec;
    s.size = {4.572f, 1.900f};
    s.wheelBase = 2.457f;
    s.weight = 1450.0f;
    s.wheelDiameter = 0.734f;                  // 335/30 R21 (arriere, motrices)
    s.gearRatios = {-3.420f, 0.0f, 3.750f, 2.380f, 1.720f, 1.340f, 1.110f, 0.960f, 0.840f};
    s.maxGear = 7;
    s.finalDrive = 4.27f;
    s.driveFrontShare = 0.0f;                  // propulsion
    s.clutchMaxTorque = 700.0f;
    s.steerSpeedRef = 20.0f;
    s.maxBrakeForce = 17000.0f;
    s.CdA = 0.86f;                             // Cd 0.39 x 2.21 m2
    s.tireGrip = 1.4f;                         // pneus Cup
    s.tractionControl = 2.0f;                  // marge de 20 % : propulsion tres puissante, sinon elle decroche
    deriveChassis(s, 0.39f, 1.3f);             // moteur en porte-a-faux arriere
    s.cornerStiffRearRatio = 1.8f;             // pneus arriere 335 contre 275 a l'avant : sans ca la voiture survire et devient instable vers 150 km/h

    // 4.0 L atmospherique : 386 kW a 8500 tr/min, 465 Nm a 6300 tr/min
    Engine::Config& e = m.engine;
    e.idleRPM = 900.0f;
    e.maxRPM = 9000.0f;
    e.maxTorque = 465.0f;
    e.inertia = 0.14f;
    e.frictionTorque = 9.0f;
    e.frictionPerRPM = 0.005f;
    e.starterTorque = 100.0f;
    e.compressionTorque = 40.0f;
    e.torqueCurve = {{
        {0.0f, 0.45f}, {1500.0f, 0.60f}, {3000.0f, 0.75f}, {4500.0f, 0.88f},
        {6300.0f, 1.00f}, {7500.0f, 0.97f}, {8500.0f, 0.93f}, {9000.0f, 0.85f}
    }};

    m.sound.cylinders = 6;
    m.sound.firingOrder = {0, 5, 1, 3, 2, 4, 0, 0};  // 6 a plat, 1-6-2-4-3-5
    m.sound.resonanceHz = 150.0f;
    m.sound.brightness = 1.5f;                       // atmospherique a haut regime : son mordant
    m.sound.damping = 0.7f;
    return m;
}

CarModel chiron() {
    CarModel m;
    m.name = "Bugatti Chiron";
    m.sprite = "Cars/Chiron.png";

    CarSpec& s = m.spec;
    s.size = {4.544f, 2.038f};
    s.wheelBase = 2.711f;
    s.weight = 1995.0f;
    s.wheelDiameter = 0.711f;                  // 355/25 R21 (arriere)
    s.gearRatios = {-3.000f, 0.0f, 2.950f, 2.300f, 1.800f, 1.410f, 1.100f, 0.860f, 0.660f};
    s.maxGear = 7;
    s.finalDrive = 3.1f;
    s.driveFrontShare = 0.35f;                 // transmission integrale a dominante arriere
    s.clutchMaxTorque = 2000.0f;
    s.steerSpeedRef = 25.0f;
    s.maxBrakeForce = 17000.0f;
    s.CdA = 0.76f;
    s.tireGrip = 1.35f;
    deriveChassis(s, 0.44f, 1.3f);             // moteur central
    s.cornerStiffRearRatio = 1.5f;             // pneus arriere 355 contre 285 a l'avant

    // W16 8.0 L quadri-turbo : 1103 kW a 6700 tr/min, 1600 Nm de 2000 a 6000 tr/min
    Engine::Config& e = m.engine;
    e.idleRPM = 800.0f;
    e.maxRPM = 7000.0f;
    e.maxTorque = 1600.0f;
    e.inertia = 0.35f;
    e.frictionTorque = 15.0f;
    e.frictionPerRPM = 0.008f;
    e.starterTorque = 200.0f;                  // doit vaincre 70 Nm de compression : regime de virage 400 x (1 - 70/200) = 260 tr/min
    e.compressionTorque = 70.0f;
    e.torqueCurve = {{
        {0.0f, 0.45f}, {1000.0f, 0.70f}, {2000.0f, 1.00f}, {4500.0f, 1.00f},
        {6000.0f, 1.00f}, {6700.0f, 0.98f}, {6850.0f, 0.95f}, {7000.0f, 0.90f}
    }};

    m.sound.cylinders = 16;
    m.sound.firingOrder = {0, 13, 8, 3, 6, 11, 14, 5, 12, 7, 2, 15, 10, 1, 4, 9};  // W16, 1-14-9-4-7-12-15-6-13-8-3-16-11-2-5-10
    m.sound.resonanceHz = 70.0f;
    m.sound.brightness = 0.5f;                       // 16 allumages par tour : sans ca le son siffle
    m.sound.damping = 3.0f;                          // allumages longs qui se fondent en un grondement
    m.sound.volume = 0.95f;                          // le filtrage et la limitation des voix baissent le niveau
    return m;
}

}  // namespace

const std::vector<CarModel>& carCatalog() {
    static const std::vector<CarModel> catalog = {
        twingo(), subaruWrx(), skylineR34(), renegade4xe(), gt3Rs(), chiron(),
    };
    return catalog;
}
