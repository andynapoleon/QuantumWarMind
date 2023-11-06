#include "BasicSc2Bot.h"
#include <iostream>

#define DEBUG_MODE true
#define VERBOSE false

using namespace sc2;

void BasicSc2Bot::OnGameStart() { if (VERBOSE) { std::cout << "It's Gamin time" << std::endl; } }

void BasicSc2Bot::OnStep() {
    TryBuildExtractor();
    TryBuildSpawningPool();
    // std::cout << Observation()->GetArmyCount() << std::endl;
    // std::cout << Observation()->GetGameLoop() << std::endl;
    // std::cout << Observation()->GetMinerals() << std::endl;
    // std::cout << Observation()->GetVespene() << std::endl;
    }

void BasicSc2Bot::OnUnitCreated(const Unit* unit) {
    if (VERBOSE) {
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
}

bool BasicSc2Bot::TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_to_build_type) {
    const ObservationInterface* observation = Observation();

    // If a unit already is building a supply structure of this type, do nothing.
    // Also get an drone to build the structure.
    const Unit* unit_to_build = nullptr;
    Units units = observation->GetUnits(Unit::Alliance::Self);
    for (const auto& unit : units) {
        for (const auto& order : unit->orders) {
            if (order.ability_id == ability_type_for_structure) {
                return false;
            }
        }
        if (unit->unit_type == unit_to_build_type) {
            unit_to_build = unit;
        }
    }
    if (ability_type_for_structure == ABILITY_ID::BUILD_EXTRACTOR) {
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, FindNearestExtractor(ability_type_for_structure));
    }
    else {
        Point2D build_location = FindBuildLocation(ability_type_for_structure);
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, build_location);
    }
    return true;
}

void BasicSc2Bot::OnUnitIdle(const Unit* unit) {
    switch (unit->unit_type.ToType()) {
        case UNIT_TYPEID::ZERG_LARVA: {
            // When we make our 13th drone we need to make a Overlord
            size_t sum_overlords = CountUnitType(UNIT_TYPEID::ZERG_OVERLORD) + CountEggUnitsInProduction(ABILITY_ID::TRAIN_OVERLORD);
            size_t sum_drones = CountUnitType(UNIT_TYPEID::ZERG_DRONE) + CountEggUnitsInProduction(ABILITY_ID::TRAIN_DRONE);
            if ((sum_overlords < 2) && (sum_drones == 13)) {
                if (DEBUG_MODE) {
                    std::cout << "We have reached a checkpoint. We have " << sum_overlords << " overlords & " << sum_drones << " drones we will try to train another overlord" << std::endl;
                }
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_OVERLORD);
            } else {
                Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_DRONE);
            }
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

void BasicSc2Bot::TryBuildExtractor() {
    const ObservationInterface* observation = Observation();
    const Units& drones = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_DRONE));
    const Units& extractors = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_EXTRACTOR));
    // we only need 1 extractor, we do not want to build more than we need
    if (drones.size() < 16 || !extractors.empty()) {
        return;
    }
    // Issue the build command to build an extractor on the geyser
    if (DEBUG_MODE) {
        std::cout << "We have reached a checkpoint. We have " << drones.size() << " drones & " << extractors.size() << " extractors we will try to build a extractor" << std::endl;
    }
    TryBuildStructure(ABILITY_ID::BUILD_EXTRACTOR);
}

bool BasicSc2Bot::TryBuildSpawningPool() {
    const ObservationInterface* observation = Observation();
    std::vector<sc2::Tag> units_with_commands = Actions()->Commands();
    if (observation->GetMinerals() < 200 || CountUnitType(UNIT_TYPEID::ZERG_EXTRACTOR) < 1) {
        return false;
    }
    if (CountUnitType(UNIT_TYPEID::ZERG_SPAWNINGPOOL) > 0) {
        return true;
    }

    if (DEBUG_MODE) {
        std::cout << "We have reached a checkpoint. We will try to build a spawning pool" << std::endl;
    }
    return TryBuildStructure(ABILITY_ID::BUILD_SPAWNINGPOOL);
}

//helper function
size_t BasicSc2Bot::CountUnitType(UNIT_TYPEID unit_type) {
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

// helper function that counts number of a certain unit currently in production
// takes in the name of the ability that makes the unit, ex ABILITY::TRAIN_OVERLORD
size_t BasicSc2Bot::CountEggUnitsInProduction(ABILITY_ID unit_ability) {
    Units units = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_EGG));
    size_t unit_count = 0;
    for (const auto& unit : units) {
        if (unit->orders.front().ability_id == unit_ability) {
            ++unit_count;
        }
    }
    return unit_count;
}

Point2D BasicSc2Bot::FindBuildLocation(ABILITY_ID unit_ability) {
    const ObservationInterface* observation = Observation();
    const Units& hatcherys = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_HATCHERY));

    // Check if you have at least one Hatchery
    if (hatcherys.empty()) {
        if (DEBUG_MODE) {
            std::cout << "We have no hatcherys" << std::endl;
        }
        return Point2D(0.0f, 0.0f);  // Return an invalid location
    }

    const Unit* hatchery = hatcherys.front();  // Use the first Hatchery
    
    // Find a suitable location nearby the Hatchery
    // You can use the position of the Hatchery to calculate the location
    Point2D buildLocation;
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    buildLocation = Point2D(hatchery->pos.x + rx * 5.0f, hatchery->pos.y + ry * 5.0f); // Example: Build 3 units to the right of the Hatchery

    // You should add more logic to check if the build location is valid, such as ensuring it's on creep or open ground.
    if (DEBUG_MODE) {
        std::cout << "We will build at (" << buildLocation.x << "," << buildLocation.y << ")" << std::endl;
    }
    return buildLocation;
}

const Unit* BasicSc2Bot::FindNearestExtractor(ABILITY_ID unit_ability) {
    const ObservationInterface* observation = Observation();
    const Units& hatcherys = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_HATCHERY));

    const Unit* hatchery = hatcherys.front();  // Use the first Hatchery

    if (unit_ability == ABILITY_ID::BUILD_EXTRACTOR) {  // if we are trying to build an extractor we want the closest one
        const Units& geysers = Observation()->GetUnits(Unit::Alliance::Neutral, IsUnit(UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));
        const Unit* closest_geyser = geysers.front();
        for (const auto& geyser : geysers) {

            if (Distance3D(hatchery->pos, closest_geyser->pos) > Distance3D(hatchery->pos, geyser->pos)) {
                closest_geyser = geyser;
            }
        }
        return closest_geyser;
    }
}
