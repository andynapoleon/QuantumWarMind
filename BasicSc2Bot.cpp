#include "BasicSc2Bot.h"
#include <iostream>

using namespace sc2;

void BasicSc2Bot::OnGameStart() { std::cout << "Hello, World!" << std::endl; }

void BasicSc2Bot::OnStep() {
    // TryBuildSupplyDepot();
    // TryBuildBarracks();
    TryBuildExtractor();
    // TryBuildSpawningPool();
    // std::cout << Observation()->GetArmyCount() << std::endl;
    // std::cout << Observation()->GetGameLoop() << std::endl;
    // std::cout << Observation()->GetMinerals() << std::endl;
    // std::cout << Observation()->GetVespene() << std::endl;
    }

void BasicSc2Bot::OnUnitCreated(const Unit* unit) {
    if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_OVERLORD) {
        std::cout << "Overlord created with tag: " << unit->tag << std::endl;
    }
    if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_DRONE) {
        std::cout << "Drone created with tag: " << unit->tag << std::endl;
    }
    if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_EXTRACTOR) {
        std::cout << "Extractor created with tag: " << unit->tag << std::endl;
    }
    if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL) {
        std::cout << "Spawingpool created with tag: " << unit->tag << std::endl;
    }
}

bool BasicSc2Bot::TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type) {
    const ObservationInterface* observation = Observation();

    // If a unit already is building a supply structure of this type, do nothing.
    // Also get an scv to build the structure.
    const Unit* unit_to_build = nullptr;
    Units units = observation->GetUnits(Unit::Alliance::Self);
    for (const auto& unit : units) {
        for (const auto& order : unit->orders) {
            if (order.ability_id == ability_type_for_structure) {
                return false;
            }
        }
        if (unit->unit_type == UNIT_TYPEID::ZERG_DRONE) {
            unit_to_build = unit;
        }
    }

    Actions()->UnitCommand(unit_to_build, ability_type_for_structure, FindBuildLocation());
    return true;
}


// bool BasicSc2Bot::TryBuildSupplyDepot(){
//     const ObservationInterface* observation = Observation();

//     // If we are not supply capped, don't build a supply depot.
//     if (observation->GetFoodUsed() <= observation->GetFoodCap() - 2){
//         return false;
//     }
    
//     // Try and build a depot. Find a random SCV and give it the order.
//     return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT);
// }


void BasicSc2Bot::OnUnitIdle(const Unit* unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::ZERG_LARVA: {
            if ((CountUnitType(UNIT_TYPEID::ZERG_OVERLORD) < 2) && (CountUnitType(UNIT_TYPEID::ZERG_DRONE) == 13)) {
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_OVERLORD);
            }
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_DRONE);
            break;
        }
        case UNIT_TYPEID::ZERG_DRONE: {
            const Unit* mineral_target = FindNearestMineralPatch(unit->pos);
            if (!mineral_target) {
                break;
            }
            Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
            
            break;
        }
        // case UNIT_TYPEID::TERRAN_BARRACKS: {
        //     Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
        //     break;
        // }
        case UNIT_TYPEID::ZERG_OVERLORD: {
            const GameInfo& game_info = Observation()->GetGameInfo();
            Actions()->UnitCommand(unit, ABILITY_ID::MOVE_MOVEPATROL, game_info.enemy_start_locations.front());
            break;
        }
        default : {
            break;
        }
    }
}

const Unit* BasicSc2Bot::FindNearestMineralPatch(const Point2D& start) {
    Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
    float distance = std::numeric_limits<float>::max();
    const Unit* target = nullptr;
    for (const auto& u : units) {
        if (u->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD) {
            float d = DistanceSquared2D(u->pos, start);
            if (d < distance) {
                distance = d;
                target = u;
            }
        }
    }
    return target;
}

// bool BasicSc2Bot::TryBuildBarracks() {
//     const ObservationInterface* observation = Observation();
//     if (CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 1) {
//         return false;
//     }
//     if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) > 0) {
//         return true;
//     }
//     return TryBuildStructure(ABILITY_ID::BUILD_BARRACKS);
// }

void BasicSc2Bot::TryBuildExtractor() {
    const ObservationInterface* observation = Observation();
    const Units& drones = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_DRONE));
    const Units& geysers = Observation()->GetUnits(Unit::Alliance::Neutral, IsUnit(UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));

    // Choose a drone and a geyser to build the extractor
    if (!drones.empty() && !geysers.empty()) {
        const Unit* drone = drones.front(); // Choose the first available drone
        const Unit* geyser = geysers.front(); // Choose the first available geyser

        // Issue the build command to build an extractor on the geyser
        Actions()->UnitCommand(drone, ABILITY_ID::BUILD_EXTRACTOR, geyser);
    }
}

bool BasicSc2Bot::TryBuildSpawningPool() {
    const ObservationInterface* observation = Observation();
    if (observation->GetMinerals() < 200) {
        return false;
    }
    if (CountUnitType(UNIT_TYPEID::ZERG_SPAWNINGPOOL) > 0) {
        return true;
    }
    return TryBuildStructure(ABILITY_ID::BUILD_SPAWNINGPOOL);
}

//helper function
size_t BasicSc2Bot::CountUnitType(UNIT_TYPEID unit_type) {
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

Point2D BasicSc2Bot::FindBuildLocation() {
    const ObservationInterface* observation = Observation();
    const Units& extractors = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_EXTRACTOR));

    // Check if you have at least one Hatchery
    if (extractors.empty()) {
        return Point2D(0.0f, 0.0f);  // Return an invalid location
    }

    const Unit* extractor = extractors.front();  // Use the first Hatchery

    // Find a suitable location nearby the Hatchery
    // You can use the position of the Hatchery to calculate the location
    Point2D buildLocation = extractor->pos + Point2D(3.0f, 0.0f); // Example: Build 3 units to the right of the Hatchery

    // You should add more logic to check if the build location is valid, such as ensuring it's on creep or open ground.

    return buildLocation;
}
