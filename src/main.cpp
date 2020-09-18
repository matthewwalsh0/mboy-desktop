#include <sys/types.h>
#include <thread>

#include "DesktopGUI.h"
#include "Gameboy.h"
#include "SDLRenderer.h"
#include "SDLDebugRenderer.h"
#include "argparse.hpp"

static void startEmulator(DesktopGUI* gui, std::string romPath, Config* config)
{
    Gameboy gameboy = Gameboy(romPath, (GUI*) gui, config);
    gameboy.run();
}

static argparse::ArgumentParser parseArgs(int argc, char* argv[]) {
    argparse::ArgumentParser program("mboy");

    program.add_argument("rom")
        .help("gb or gbc rom file to execute");

    program.add_argument("-d", "--debug")
        .help("enables debug mode to display additional info")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
        return program;
    } catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << program;
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    argparse::ArgumentParser args = parseArgs(argc, argv);

    Renderer* renderer = args.get<bool>("--debug") ?
        (Renderer*) new SDLDebugRenderer() :
        (Renderer*) new SDLRenderer();

    std::string romPath (args.get<std::string>("rom"));
    Config emulatorConfig;
    DesktopGUI gui(&emulatorConfig);
    std::thread emulatorThread (startEmulator, &gui, romPath, &emulatorConfig);

    while(true)
    {
        if(!renderer->draw(&gui)) break;
    }

    gui.stop();
    emulatorThread.join();

    return 0;
}