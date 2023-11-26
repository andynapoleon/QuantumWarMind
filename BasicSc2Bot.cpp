#include "BasicSc2Bot.h"
#include <iostream>

#define DEBUG_MODE true
#define VERBOSE false

using namespace sc2;
using namespace std;

/*
===========================================================================
                            SC2 API FUNCTIONS
===========================================================================
*/
void BasicSc2Bot::OnGameStart()
{
    if (VERBOSE)
    {
        std::cout << "It's Gamin time" << std::endl;
    }
}

void BasicSc2Bot::OnStep()
{
    TryBuildExtractor();
    TryBuildSpawningPool();
    // TryCreateZergQueen();
    // TryFillGasExtractor();
}

void BasicSc2Bot::OnUnitCreated(const Unit *unit)
{
    if (VERBOSE)
    {
        if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_OVERLORD)
        {
            std::cout << "Overlord created with tag: " << unit->tag << std::endl;
        }
        if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_DRONE)
        {
            std::cout << "Drone created with tag: " << unit->tag << std::endl;
        }
        if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_EXTRACTOR)
        {
            std::cout << "Extractor created with tag: " << unit->tag << std::endl;
        }
        if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL)
        {
            std::cout << "Spawingpool created with tag: " << unit->tag << std::endl;
        }
        if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_HATCHERY)
        {
            std::cout << "Hatcher created with tag: " << unit->tag << std::endl;
        }
    }
}

void BasicSc2Bot::OnUnitIdle(const Unit *unit)
{
    switch (unit->unit_type.ToType())
    {
    case UNIT_TYPEID::ZERG_LARVA:
    {
        // Get the # of supply used and # of drones and overlords on field + in production
        int usedSupply = Observation()->GetFoodUsed();
        size_t sum_overlords = CountUnitType(UNIT_TYPEID::ZERG_OVERLORD) + CountEggUnitsInProduction(ABILITY_ID::TRAIN_OVERLORD);
        size_t sum_drones = CountUnitType(UNIT_TYPEID::ZERG_DRONE) + CountEggUnitsInProduction(ABILITY_ID::TRAIN_DRONE);

        // What to do with larva when idle?
        if (sum_overlords < 2 && sum_drones == 13) // when we make our 13th drone we need to make an Overlord
        {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_OVERLORD);
        }
        else if (usedSupply == 19) // when we reach supply 19 we need to make an Overlord
        {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_OVERLORD);
        }
        else if (usedSupply < 17) // cap # of drones at 17
        {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_DRONE);
        }
        break;
    }
    case UNIT_TYPEID::ZERG_DRONE:
    {
        const Unit *mineral_target = FindNearestMineralPatch(unit->pos);
        if (!mineral_target)
        {
            break;
        }
        Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
        break;
    }
    case UNIT_TYPEID::ZERG_OVERLORD:
    {
        const GameInfo &game_info = Observation()->GetGameInfo();
        Actions()->UnitCommand(unit, ABILITY_ID::MOVE_MOVEPATROL, game_info.enemy_start_locations.front());
        break;
    }
    default:
    {
        break;
    }
    }
}

/*
===========================================================================
                            ON-STEP FUNCTIONS
===========================================================================
*/
void BasicSc2Bot::TryBuildExtractor()
{
    // Get drones and extractors and # of supply used
    const ObservationInterface *observation = Observation();
    const Units &drones = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_DRONE));
    const Units &extractors = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_EXTRACTOR));
    int usedSupply = Observation()->GetFoodUsed();

    // We only need 1 extractor at supply 17, we do not want to build more than we need
    if (!extractors.empty() || usedSupply != 17)
    {
        return;
    }

    // Issue the build command to build an extractor on the geyser
    if (DEBUG_MODE)
    {
        std::cout << "We have reached a checkpoint. We have " << drones.size() << " drones & " << extractors.size() << " extractors we will try to build a extractor" << std::endl;
    }
    TryBuildStructure(ABILITY_ID::BUILD_EXTRACTOR);
}

void BasicSc2Bot::TryBuildSpawningPool()
{
    const ObservationInterface *observation = Observation();
    int usedSupply = Observation()->GetFoodUsed();

    // We only build pool when we have enough minerals after extractor's completion at supply 17
    if (observation->GetMinerals() < 200 || CountUnitType(UNIT_TYPEID::ZERG_EXTRACTOR) < 1 || usedSupply != 17)
    {
        return;
    }

    // We only build pool ONCE
    if (CountUnitType(UNIT_TYPEID::ZERG_SPAWNINGPOOL) > 0)
    {
        return;
    }

    // Issue the build command to build an spawning pool
    if (DEBUG_MODE)
    {
        std::cout << "We have reached a checkpoint. We will try to build a spawning pool" << std::endl;
    }
    TryBuildStructure(ABILITY_ID::BUILD_SPAWNINGPOOL);
}

// void BasicSc2Bot::TryExpandNewHatchery()
// {
// }

void BasicSc2Bot::TryCreateZergQueen()
{
    const Units &hatcheries = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_HATCHERY));
    bool making_queen = false;
    for (const auto &hatchery : hatcheries)
    {
        if (GetQueensInQueue(hatchery) > 0)
        {
            making_queen = true;
        }
    }
    if (CountUnitType(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL) > 0 &&
        CountUnitType(sc2::UNIT_TYPEID::ZERG_HATCHERY) > CountUnitType(sc2::UNIT_TYPEID::ZERG_QUEEN) &&
        !making_queen)
    {
        Actions()->UnitCommand(hatcheries.front(), sc2::ABILITY_ID::TRAIN_QUEEN);
    }
}

void BasicSc2Bot::TryFillGasExtractor()
{
    std::vector<const sc2::Unit *> mineralGatheringDrones = GetMineralGatheringDrones();
    const Units &extractors = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_EXTRACTOR));
    for (const auto &extractor : extractors)
    {
        if (!extractor || extractor->build_progress < 1.0f || extractor->assigned_harvesters > 0)
        {
            return;
        }

        if (DEBUG_MODE)
        {
            std::cout << "Attempting to fill gas extractor: " << extractor->tag << std::endl;
        }
        int max_drones = extractor->ideal_harvesters; // we only ever need 3 drones to fill an extractor to max capacity
        while (max_drones > 0)
        {
            const auto &drone = mineralGatheringDrones.back();
            sc2::ActionInterface *actions = Actions();
            actions->UnitCommand(drone, sc2::ABILITY_ID::HARVEST_GATHER, extractor); // just send our first 3 drones in the list to go get gas
            --max_drones;
            mineralGatheringDrones.pop_back();
        }
    }
}

/*
===========================================================================
                            HELPER FUNCTIONS
===========================================================================
*/
bool BasicSc2Bot::TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_to_build_type)
{
    const ObservationInterface *observation = Observation();

    // If a unit already is building a supply structure of this type, do nothing.
    // Also get an drone to build the structure.
    const Unit *unit_to_build = nullptr;
    Units units = observation->GetUnits(Unit::Alliance::Self);
    for (const auto &unit : units)
    {
        for (const auto &order : unit->orders)
        {
            if (order.ability_id == ability_type_for_structure)
            {
                return false;
            }
        }
        if (unit->unit_type == unit_to_build_type)
        {
            unit_to_build = unit;
        }
    }
    if (ability_type_for_structure == ABILITY_ID::BUILD_EXTRACTOR)
    {
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, FindNearestExtractor(ability_type_for_structure));
    }
    else
    {
        Point2D build_location = FindNearestBuildLocation(UNIT_TYPEID::ZERG_HATCHERY);
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, build_location);
    }
    return true;
}

const Unit *BasicSc2Bot::FindNearestMineralPatch(const Point2D &start)
{
    Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
    float distance = std::numeric_limits<float>::max();
    const Unit *target = nullptr;
    for (const auto &u : units)
    {
        if (u->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD && u->pos.x != start.x && u->pos.y != start.y)
        {
            float d = DistanceSquared2D(u->pos, start);
            if (d < distance)
            {
                distance = d;
                target = u;
            }
        }
    }
    return target;
}

size_t BasicSc2Bot::CountUnitType(UNIT_TYPEID unit_type)
{
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

// helper function that counts number of a certain unit currently in production
// takes in the name of the ability that makes the unit, ex ABILITY::TRAIN_OVERLORD
size_t BasicSc2Bot::CountEggUnitsInProduction(ABILITY_ID unit_ability)
{
    Units units = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_EGG));
    size_t unit_count = 0;
    for (const auto &unit : units)
    {
        if (unit->orders.front().ability_id == unit_ability)
        {
            ++unit_count;
        }
    }
    return unit_count;
}

Point2D BasicSc2Bot::FindNearestBuildLocation(sc2::UNIT_TYPEID type_)
{
    const ObservationInterface *observation = Observation();
    const Units &units = observation->GetUnits(Unit::Alliance::Self, IsUnit(type_));

    // Check if you have at least one unit
    if (units.empty())
    {
        if (DEBUG_MODE)
        {
            std::cout << "We have no units" << std::endl;
        }
        return Point2D(0.0f, 0.0f); // Return an invalid location
    }

    const Unit *unit = units.front(); // Use the first units

    // Find a suitable location nearby the units
    // You can use the position of the units to calculate the location
    Point2D buildLocation;
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    buildLocation = Point2D(unit->pos.x + rx * 5.0f, unit->pos.y + ry * 5.0f); // Example: Build 3 units to the right of the Hatchery

    // You should add more logic to check if the build location is valid, such as ensuring it's on creep or open ground.
    if (DEBUG_MODE)
    {
        std::cout << "We will build at (" << buildLocation.x << "," << buildLocation.y << ")" << std::endl;
    }
    return buildLocation;
}

const Unit *BasicSc2Bot::FindNearestExtractor(ABILITY_ID unit_ability)
{
    const ObservationInterface *observation = Observation();
    const Units &units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_HATCHERY));

    const Unit *hatchery = units.front(); // Use the first Hatchery

    if (unit_ability == ABILITY_ID::BUILD_EXTRACTOR)
    {
        const Units &geysers = Observation()->GetUnits(Unit::Alliance::Neutral, IsUnit(UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));
        const Unit *closest_geyser = geysers.front();
        for (const auto &geyser : geysers)
        {
            if (Distance3D(hatchery->pos, closest_geyser->pos) > Distance3D(hatchery->pos, geyser->pos))
            {
                closest_geyser = geyser;
            }
        }
        return closest_geyser;
    }
}

int BasicSc2Bot::GetQueensInQueue(const sc2::Unit *hatchery)
{
    int queensInQueue = 0;
    if (hatchery)
    {
        const auto &orders = hatchery->orders;

        for (const auto &order : orders)
        {
            if (order.ability_id == sc2::ABILITY_ID::TRAIN_QUEEN)
            {
                queensInQueue++;
            }
        }
    }
    return queensInQueue;
}

std::vector<const sc2::Unit *> BasicSc2Bot::GetMineralGatheringDrones()
{
    std::vector<const sc2::Unit *> mineralGatheringDrones;
    std::vector<const sc2::Unit *> units = Observation()->GetUnits(sc2::Unit::Alliance::Self);

    for (const auto &unit : units)
    {
        if (unit->orders.size() > 0)
        {
            if (unit->orders[0].ability_id == sc2::ABILITY_ID::HARVEST_GATHER)
            {
                mineralGatheringDrones.push_back(unit);
            }
        }
    }
    return mineralGatheringDrones;
}