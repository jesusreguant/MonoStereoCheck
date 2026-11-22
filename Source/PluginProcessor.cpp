#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout MonoCheckProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 }, "Umbral",
        juce::NormalisableRange<float> (thresholdMinDb, thresholdMaxDb, 0.5f),
        thresholdDefaultDb,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "reset", 1 }, "Reset", false));

    return layout;
}

MonoCheckProcessor::MonoCheckProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MonoCheckState", createLayout())
{
    thresholdParam = apvts.getRawParameterValue ("threshold");
    resetParam     = apvts.getRawParameterValue ("reset");
    lastResetOn = resetParam->load() > 0.5f;
}

void MonoCheckProcessor::prepareToPlay (double sampleRate, int)
{
    sampleRateHz      = sampleRate;
    minMonoSamples    = (juce::int64) (sampleRate * minMonoSeconds);
    signalHoldSamples = (juce::int64) (sampleRate * signalHoldSeconds);
    silenceCounter    = 0;
    monoRunSamples    = 0;
    hasSignal         = false;
    lastResetOn       = resetParam->load() > 0.5f;
    peakL = 0.0f;
    peakR = 0.0f;
}

bool MonoCheckProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void MonoCheckProcessor::resetLatch()
{
    monoLatched = false;
    monoRunSamples = 0;
    currentState = NoSignal;
}

// Un bloque mono suma al contador; al llegar al tiempo mínimo se retiene en rojo.
// Un bloque estéreo reinicia el contador.
void MonoCheckProcessor::registerBlock (bool isMono, int numSamples)
{
    if (isMono)
    {
        const auto run = monoRunSamples.load() + numSamples;
        monoRunSamples = run;

        if (run >= minMonoSamples)
        {
            currentState = Mono;
            monoLatched = true;
        }
    }
    else
    {
        monoRunSamples = 0;
        currentState = Stereo;
    }
}

void MonoCheckProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();

    // El audio pasa sin modificarse (plugin solo de medición)

    // Reset pedido desde el DAW (parámetro automatizable): cada cambio de valor resetea
    const bool resetOn = resetParam->load() > 0.5f;
    if (resetOn != lastResetOn)
    {
        lastResetOn = resetOn;
        resetLatch();
    }

    // Umbral de detección elegido por el usuario
    const float signalThreshold = juce::Decibels::decibelsToGain (thresholdParam->load(), -120.0f);

    // Pista mono: no hay canal R, se usa L para ambos
    const bool monoBus = getTotalNumInputChannels() < 2 || buffer.getNumChannels() < 2;

    const float* l = buffer.getReadPointer (0);
    const float* r = monoBus ? l : buffer.getReadPointer (1);

    float peakLBlock = 0.0f, peakRBlock = 0.0f, side = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        peakLBlock = juce::jmax (peakLBlock, std::abs (l[i]));
        peakRBlock = juce::jmax (peakRBlock, std::abs (r[i]));
        side       = juce::jmax (side,       std::abs (l[i] - r[i]));
    }

    // Picómetro: sube al instante y cae unos 20 dB por segundo
    const float decay = (float) std::pow (10.0, -(double) n / sampleRateHz);
    peakL = juce::jmax (peakLBlock, peakL.load() * decay);
    peakR = juce::jmax (peakRBlock, peakR.load() * decay);

    const float peak = juce::jmax (peakLBlock, peakRBlock);

    if (peak >= signalThreshold)
    {
        silenceCounter = 0;
        hasSignal = true;

        // En señales muy bajas la diferencia L-R se compara también con el propio nivel (-40 dB),
        // para no confundir un estéreo muy bajo con mono
        const float sideLimit = juce::jmin (sideThreshold, peak * 0.01f);
        registerBlock (side < sideLimit, n);
    }
    else
    {
        silenceCounter += n;

        if (silenceCounter >= signalHoldSamples)
        {
            hasSignal = false;

            // Sin señal: el rojo retenido se conserva; si no, vuelve a "No audio"
            if (! monoLatched.load() && monoRunSamples.load() == 0)
                currentState = NoSignal;
        }
    }
}

void MonoCheckProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MonoCheckProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* MonoCheckProcessor::createEditor()
{
    return new MonoCheckEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MonoCheckProcessor();
}
