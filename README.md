# WarwickMotoControlAndSimulationTechnicalTask

# Thanks for taking the time to read this, and explore my repository. 
# It contains 4 folders, consisting of the Warwick Moto: 
# - Matlab task
# - Simulink task
# - DBC editing task
# - Personal Projects

# Notes for matlab task
#
# Please load the data files into the same folder as the source code file. This is required to load in the data directly.
#

# Notes for simulink task
#
# I have modelled a plant containing the required ABS system. Direction for use included in the file, including plots. 
# Please run the corresponding Matlab file included to load in the constants. 
# I modelled the plant to include some core features, including drag, dynamic load transfer, individual wheel dynamics. I also temporarily included a throttle, allowing you to reset the desired speed without having to reset the simulation run, although I haven't added a gearbox or traction control yet. All assumed constant values, such as torque numbers, wheelbase, centre of mass height, frontal exposed area to oncoming airflow, I have just pulled from a google search. Please feel free to adjust the plant. 

# Notes for DBC task
#

# Notes for personal task
#
# The task included contains the code for an STM32G071CBT6 MCU, designed to work alongside a CP2102N interface chip, take sensor values from a MAX31855KASA+ chip, process the data, and either compute PWM values and pass them over to a lab view control system. Unfortunately, I am unsure how much I am allowed to share so some information has been commented out. The whole codes logic can be summarised by the main loop found on line 500. If you have any questions regarding the code, or any other aspects please let me know. 
