################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
hardware/car_app/%.o: ../hardware/car_app/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Arm Compiler - building file: "$<"'
	"D:/ti/ccs2050/ccs/tools/compiler/ti-cgt-armllvm_4.0.4.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O2 -I"C:/Users/WHY/Desktop/ProgramForCar/MOTOR_V1_1_2/hardwire/track_exclude" -I"C:/Users/WHY/Desktop/ProgramForCar/MOTOR_V1_1_2/hardwire/motor" -I"C:/Users/WHY/Desktop/ProgramForCar/MOTOR_V1_1_2" -I"C:/Users/WHY/Desktop/ProgramForCar/MOTOR_V1_1_2/Debug" -I"D:/ti/mspm0_sdk_2_10_00_04/source/third_party/CMSIS/Core/Include" -I"D:/ti/mspm0_sdk_2_10_00_04/source" -gdwarf-3 -Wall -MMD -MP -MF"hardware/car_app/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


