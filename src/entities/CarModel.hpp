#pragma once
#include "raylib.h"
#include "../audio/EngineSynth.hpp"
#include "Engine.hpp"
#include <array>
#include <string>

// Parametres fixes d'une voiture. Les valeurs par defaut sont celles de la Subaru WRX (VB, BVM6).
struct CarSpec {
    // Dimensions et masse
    Vector2 size = {4.67f, 1.83f};          // longueur x largeur (m)
    float wheelBase = 2.67f;                // empattement (m)
    float bodyCenterFromRearAxle = 1.25f;   // centre de la carrosserie par rapport a l'essieu arriere (m)
    float weight = 1530.0f;                 // kg
    float wheelDiameter = 0.653f;           // m (245/40 R18)

    // Transmission. Rapports reels de boite, indices : 0 = R (negatif), 1 = N (0), 2..8 = 1re..7e
    std::array<float, 9> gearRatios = {-3.636f, 0.0f, 3.454f, 1.947f, 1.296f, 0.972f, 0.780f, 0.666f, 0.0f};
    int maxGear = 6;
    float finalDrive = 4.111f;              // rapport de pont
    float driveFrontShare = 0.5f;           // part du couple moteur envoyee a l'essieu avant (1 = traction, 0 = propulsion)
    float clutchMaxTorque = 500.0f;         // Nm
    float clutchBitePoint = 0.3f;           // position d'embrayage au-dela de laquelle la boite est debrayee

    // Direction, freinage, aerodynamique
    float maxSteeringAngle = 30.0f * DEG2RAD;  // braquage max des roues avant (rad)
    float steerSpeedRef = 18.0f;               // braquage divise par 2 a cette vitesse (m/s)
    float maxBrakeForce = 14000.0f;            // N (~0.95 g)
    float CdA = 0.72f;                         // m^2 (Cx x surface frontale)

    // Comportement du chassis (voir LateralParams)
    float frontWeightFrac = 0.58f;          // part du poids sur l'essieu avant
    float yawInertia = 2500.0f;             // kg*m^2
    float cornerStiffness = 120000.0f;      // N/rad, essieu avant
    float cornerStiffRearRatio = 1.0f;      // raideur arriere / avant : > 1 pour les voitures lourdes de l'arriere (sinon survirage instable a haute vitesse)
    float tireGrip = 1.0f;                  // multiplicateur d'adherence des pneus
    // Force du controle de traction : marge d'adherence gardee par essieu = 10 % x tractionControl.
    // 1 = marge de 10 %, plus haut = intervient plus tot (plus stable, accelere moins fort), 0 = aucune marge.
    float tractionControl = 1.0f;
};

// Une voiture jouable : nom, sprite, chassis, moteur et son.
struct CarModel {
    std::string name;
    std::string sprite;                     // chemin relatif a assets/
    CarSpec spec;
    Engine::Config engine;
    EngineSynth::Config sound;
};
