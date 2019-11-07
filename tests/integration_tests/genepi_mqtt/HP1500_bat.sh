: # CONFIGURE UPS HID OBJECTS

: # HP 1500
: # scenario: [UPS mode = battery| Mains = Off | load = 500W | no failure]
: # only most important objects are set here

./HidSim set UPS.BatterySystem.Battery.PresentStatus.Present 1
./HidSim set UPS.BatterySystem.Charger.Mode 2
./HidSim set UPS.BatterySystem.Charger.PresentStatus.Floating 1
./HidSim set UPS.BatterySystem.Charger.PresentStatus.InternalFailure 0
./HidSim set UPS.BatterySystem.Charger.PresentStatus.Used 0
./HidSim set UPS.BatterySystem.Charger.PresentStatus.VoltageTooHigh 0
./HidSim set UPS.BatterySystem.Charger.PresentStatus.VoltageTooLow 0

./HidSim set UPS.OutletSystem.Outlet[2].PresentStatus.SwitchOnOff 1
./HidSim set UPS.OutletSystem.Outlet[3].PresentStatus.SwitchOnOff 1

./HidSim set UPS.PowerConverter.Input[1].Frequency 50
./HidSim set UPS.PowerConverter.Input[1].PresentStatus.Boost 0
./HidSim set UPS.PowerConverter.Input[1].PresentStatus.Buck 0
./HidSim set UPS.PowerConverter.Input[1].PresentStatus.FrequencyOutOfRange 0
./HidSim set UPS.PowerConverter.Input[1].PresentStatus.InternalFailure 0
./HidSim set UPS.PowerConverter.Input[1].PresentStatus.Used 1
./HidSim set UPS.PowerConverter.Input[1].PresentStatus.VoltageOutOfRange 0
./HidSim set UPS.PowerConverter.Input[1].Voltage 240
./HidSim set UPS.PowerConverter.Input[3].PresentStatus.Used 1

./HidSim set UPS.PowerConverter.Inverter.PresentStatus.Used 1

./HidSim set UPS.PowerConverter.Output.ActivePower 500
./HidSim set UPS.PowerConverter.Output.ApparentPower 550
./HidSim set UPS.PowerConverter.Output.Current 2.4
./HidSim set UPS.PowerConverter.Output.Efficiency 0

./HidSim set UPS.PowerConverter.Output.Frequency 50
./HidSim set UPS.PowerConverter.Output.PowerFactor 91
./HidSim set UPS.PowerConverter.Output.Voltage 229.8

./HidSim set UPS.PowerSummary.PercentLoad 45
./HidSim set UPS.PowerSummary.PresentStatus.ACPresent 1
./HidSim set UPS.PowerSummary.PresentStatus.BelowRemainingCapacityLimit 0
./HidSim set UPS.PowerSummary.PresentStatus.Charging 1
./HidSim set UPS.PowerSummary.PresentStatus.CommunicationLost 0
./HidSim set UPS.PowerSummary.PresentStatus.Discharging 0
./HidSim set UPS.PowerSummary.PresentStatus.EmergencyStop 0
./HidSim set UPS.PowerSummary.PresentStatus.FanFailure 0
./HidSim set UPS.PowerSummary.PresentStatus.Good 1
./HidSim set UPS.PowerSummary.PresentStatus.InternalFailure 0
./HidSim set UPS.PowerSummary.PresentStatus.NeedReplacement 0
./HidSim set UPS.PowerSummary.PresentStatus.Overload 0
./HidSim set UPS.PowerSummary.PresentStatus.OverTemperature 0
./HidSim set UPS.PowerSummary.PresentStatus.ShutdownImminent 0

./HidSim set UPS.PowerSummary.RemainingCapacity 90
./HidSim set UPS.PowerSummary.RunTimeToEmpty 17865
./HidSim set UPS.PowerSummary.Voltage 37.8



