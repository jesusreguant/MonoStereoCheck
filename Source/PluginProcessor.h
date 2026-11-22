#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class MonoCheckProcessor : public juce::AudioProcessor
{
public:
    enum State { NoSignal = 0, Stereo = 1, Mono = 2 };

    // Umbral de detección (nivel mínimo, en dBFS, a partir del cual el detector y el medidor actúan)
    static constexpr float thresholdMinDb     = -100.0f;
    static constexpr float thresholdMaxDb     = -20.0f;
    static constexpr float thresholdDefaultDb = -90.0f;

    MonoCheckProcessor();
    ~MonoCheckProcessor() override = default;

    void prepareToPlay (double sampleRate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MonoCheck"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Parámetros (se guardan en el proyecto del DAW y se pueden automatizar):
    //   "threshold" = umbral de detección en dB
    //   "reset"     = cada cambio de valor resetea el rojo (para asignarlo a una tecla/controlador del DAW)
    juce::AudioProcessorValueTreeState apvts;

    // Lo lee el editor (hilo de interfaz)
    std::atomic<int>   currentState { NoSignal };  // último estado confirmado
    std::atomic<bool>  monoLatched  { false };     // true = mono retenido (rojo)
    std::atomic<bool>  hasSignal    { false };     // hay audio entrando
    std::atomic<float> peakL        { 0.0f };      // picómetro canal izquierdo (lineal)
    std::atomic<float> peakR        { 0.0f };      // picómetro canal derecho (lineal)

    void resetLatch();

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void registerBlock (bool isMono, int numSamples);

    // Diferencia L-R por debajo de esto = mono (-80 dBFS); en señales muy bajas se usa además
    // un límite relativo al propio nivel (ver processBlock)
    static constexpr float sideThreshold   = 0.0001f;
    // El mono debe mantenerse este tiempo seguido antes de marcarse en rojo (casi instantáneo)
    static constexpr double minMonoSeconds = 0.005;
    // Tiempo sin señal antes de mostrar "No audio"
    static constexpr double signalHoldSeconds = 0.3;

    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* resetParam     = nullptr;
    bool  lastResetOn = false;

    double sampleRateHz = 44100.0;
    juce::int64 minMonoSamples    = 240;
    juce::int64 signalHoldSamples = 13230;
    juce::int64 silenceCounter    = 0;             // solo hilo de audio
    std::atomic<juce::int64> monoRunSamples { 0 }; // muestras mono consecutivas

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonoCheckProcessor)
};
