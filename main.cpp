#include <iostream>
#include "sc2api/sc2_api.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"

#include "BasicSc2Bot.h"
#include "LadderInterface.h"

// LadderInterface allows the bot to be tested against the built-in AI or
// played against other bots
int main(int argc, char* argv[]) {
    //RunBot(argc, argv, new BasicSc2Bot(), sc2::Race::Terran);
    Coordinator coordinator;
    coordinator.LoadSettings(argc, argv);
    coordinator.SetFullScreen(false);
    coordinator.SetRealtime(false);
 
    BasicSc2Bot bot;
    coordinator.SetParticipants({
        CreateParticipant(Race::Zerg, &bot),
        CreateComputer(Race::Terran)
    });

    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapBelShirVestigeLE);

    while (coordinator.Update()){
        // Slowing down gamespeed so i can see whats happening
        sc2::SleepFor(15);
    }
    return 0;
}
