#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class MonoCheckEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit MonoCheckEditor (MonoCheckProcessor&);
    ~MonoCheckEditor() override;

    void paint (juce::Graphics&) override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    // Tamaño de referencia: solo sirve para calcular el tamaño de textos y detalles
    static constexpr float refW = 240.0f;
    static constexpr float refH = 384.0f;

    // Disposición calculada a partir del ancho y el alto reales de la ventana
    struct Layout
    {
        float u = 1.0f;                       // factor de escala para textos y detalles
        juce::Rectangle<float> panel, meterL, meterR, thresholdBar;
    };

    Layout getLayout() const;

    void timerCallback() override;
    void showKeyMenu();

    float getThresholdDb() const;
    void  setThresholdDb (float db);
    float xToThresholdDb (float x, const Layout&) const;

    static void drawMeter (juce::Graphics&, juce::Rectangle<float>, float peakDb, float holdDb, float u);
    static void updateHold (float& hold, int& timer, float db);

    MonoCheckProcessor& processor;
    juce::RangedAudioParameter* thresholdParameter = nullptr;
    std::atomic<float>*         thresholdValue     = nullptr;

    float holdDbL = -100.0f, holdDbR = -100.0f;
    int   holdTimerL = 0,    holdTimerR = 0;

    bool        draggingThreshold = false;
    bool        waitingForKey     = false;   // esperando que el usuario elija la tecla de reset
    juce::juce_wchar  resetKey          = 'Ñ';     // tecla de reset (por defecto Ñ)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonoCheckEditor)
};
