################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
pwmpru/pwmpru1.obj: ../pwmpru/pwmpru1.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: C6000 Compiler'
	"C:/ti/ccsv5/tools/compiler/c6000_7.4.4/bin/cl6x" -mv6740 --abi=coffabi -g --include_path="C:/ti/ccsv5/tools/compiler/c6000_7.4.4/include" --define=c6748 --display_error_number --diag_warning=225 --diag_wrap=off --preproc_with_compile --preproc_dependency="pwmpru/pwmpru1.pp" --obj_directory="pwmpru" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


