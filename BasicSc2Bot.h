#ifndef BASIC_SC2_BOT_H_
#define BASIC_SC2_BOT_H_

#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"
#include "sc2api/sc2_unit_filters.h"

using namespace sc2;

class BasicSc2Bot : public Agent
{
public:
    virtual void OnGameStart() final;
    virtual void OnStep() final;
    virtual void OnUnitCreated(const Unit *unit) final;
    virtual void OnUnitIdle(const Unit *unit) final;

private:
    // Private variables
    bool isLingSpeedResearched = false;
    std::vector<const Unit*> queens;
    std::unordered_map<const Unit*, bool> queenHasInjected;
    int queenCounter = 0;
    Point2D enemy_base_estimate;

    std::vector<const Unit*> hatcheries;


    // Private game-loop functions
    void TryBuildExtractor();
    void TryBuildSpawningPool();
    void TryNaturallyExpand();
    void TryCreateZergQueen();
    void TryFillGasExtractor();
    void TryResearchMetabolicBoost();
    void SpamZerglings();
    void HandleQueens();
    void HandleZerglings();
    void TryCreateOverlordAtSupply();

    // Private helper functions
    bool TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type = UNIT_TYPEID::ZERG_DRONE);
    const Unit *FindNearestMineralPatch(const Point2D &start);
    size_t CountUnitType(UNIT_TYPEID unit_type);
    size_t NumFullyMade(UNIT_TYPEID unit_type);
    size_t CountEggUnitsInProduction(ABILITY_ID unit_ability);
    Point2D FindNearestBuildLocationTo(sc2::UNIT_TYPEID type_);
    const Unit *FindNearestGeyser(ABILITY_ID unit_ability);
    int GetQueensInQueue(const sc2::Unit *hatchery);
    std::vector<const sc2::Unit *> GetCurrentHarvestingDrones();
    sc2::Point2D FindExpansionLocation(float minDistanceSquared, float maxDistanceSquared);
    float DistanceSquared2D(const sc2::Point2D &p1, const sc2::Point2D &p2);
    const sc2::Unit *GetRandomElement(const std::vector<const sc2::Unit *> &elements);
    const Unit *GetMainBaseHatcheryLocation();
    void InjectLarvae(const Unit* queen, const Unit* hatchery);
    bool IsAtMainBase(const Unit* unit);
};

#endif
