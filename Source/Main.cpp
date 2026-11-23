#include "MainComponent.h"

#if JUCE_WINDOWS
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
namespace {
std::wstring dumpPath; // Allocate before installing the exception filter.
void writeCrashDump(void* context)
{
    const HANDLE file = CreateFileW(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(file == INVALID_HANDLE_VALUE) return;
    MINIDUMP_EXCEPTION_INFORMATION fault{GetCurrentThreadId(),static_cast<EXCEPTION_POINTERS*>(context),FALSE};
    MiniDumpWriteDump(GetCurrentProcess(),GetCurrentProcessId(),file,MiniDumpNormal,&fault,nullptr,nullptr);
    CloseHandle(file);
}
}
#endif

class PolyRhythmApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "PolyRhythm Pro"; }
    const juce::String getApplicationVersion() override { return "0.2.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        const auto folder=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                              .getChildFile("PolyRhythm Pro/Diagnostics");
        if(folder.createDirectory().wasOk()) {
            auto stem=juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S")+"-"+juce::Uuid().toString().substring(0,8);
            log=std::make_unique<juce::FileLogger>(folder.getChildFile(stem+".log"),
                    getApplicationName()+" "+getApplicationVersion()+" | "+juce::SystemStats::getOperatingSystemName());
            juce::Logger::setCurrentLogger(log.get());
#if JUCE_WINDOWS
            dumpPath=folder.getChildFile(stem+".dmp").getFullPathName().toWideCharPointer();
            juce::SystemStats::setApplicationCrashHandler(writeCrashDump);
#endif
        }
        juce::LookAndFeel::setDefaultLookAndFeel(&theme);
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        juce::Desktop::getInstance().setKioskModeComponent(nullptr);
        mainWindow.reset();
        juce::Logger::writeToLog("Clean shutdown");
        juce::Logger::setCurrentLogger(nullptr);
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }
    void systemRequestedQuit() override { quit(); }

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(name, juce::Colour(0xff0c1117), DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);
            setResizable(true, true);
            centreWithSize(1280, 820);
            setResizeLimits(1180, 680, 10000, 10000);
            setVisible(true);
        }
        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
    };
    std::unique_ptr<juce::FileLogger> log;
    GlassTheme theme;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(PolyRhythmApplication)
