/*
 * calibration_helpers.cpp
 *
 *  Created on: Jan 6, 2019
 *      Author: willmitchell
 */

#include <atsr.hpp>

constexpr int32_t ViaAtsr::expoSlope[4097];
constexpr int32_t ViaAtsr::logSlope[4097];
constexpr int32_t ViaAtsr::linSlope[4097];
constexpr int32_t ViaAtsr::sigmoidSlope[4097];

void ViaAtsr::render(int32_t writePosition) {

	atsrState->step();

	uint32_t aLevel = atsrState->aLevel;
	uint32_t bLevel = atsrState->bLevel;
#ifdef BUILD_F373
	int32_t aPulseWidth = (aLevel & 15) >> 1;
	int32_t bPulseWidth = (bLevel & 15) >> 1;
#endif
	aLevel >>= 4;
	bLevel >>= 4;

	gateDelayProcess();

	int32_t loopGate = !releasing & !sustaining;
	gateLowCountdown += ((lastLoop > loopGate) & gateOn) * 8;
	lastLoop = loopGate;
	loopGate |= (gateLowCountdown > 0);
	loopGate &= !startup;
	int32_t loopGateOut = __USAT((2048 - dac3Calibration) - (loopGate * 2048), 12);

	outputs.logicA[0] = GET_ALOGIC_MASK(*assignableLogic);
	outputs.auxLogic[0] = GET_EXPAND_LOGIC_MASK(gateDelayOut);
	outputs.shA[0] = GET_SH_A_MASK((aLevel != 0) * shOn);
	outputs.shB[0] = GET_SH_B_MASK((bLevel != 0) * shOn);

	pwmCounter += 1;
	pwmCounter &= 255;

	if (runtimeDisplay) {
		setLEDA((pwmCounter < (aLevel >> 4)) | shOn);
		setLEDB((pwmCounter < (bLevel >> 4)) | shOn);
		setLEDD(loopGate);
		setLEDC(*assignableLogic);
	}

#ifdef BUILD_F373

	int32_t samplesRemaining = VIA_ATSR_BUFFER_SIZE;
	int32_t ditherCounter = 0;

	while (samplesRemaining) {

		outputs.dac1Samples[writePosition] = __USAT(aLevel + (ditherCounter < aPulseWidth), 12);
		outputs.dac2Samples[writePosition] = __USAT(bLevel + (ditherCounter < bPulseWidth), 12);
		outputs.dac3Samples[writePosition] = loopGateOut;

		ditherCounter ++;
		ditherCounter &= 7;
		samplesRemaining --;
		writePosition ++;
	}

#endif

#ifdef BUILD_VIRTUAL

	outputs.dac1Samples[0] = atsrState->aLevel >> 1;
	outputs.dac2Samples[0] = atsrState->bLevel >> 1;
	outputs.dac3Samples[0] = loopGateOut;

#endif

	setLogicOut(0, 0);

}

