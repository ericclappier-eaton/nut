#!/bin/bash

### Get the directory where the script is launched
SCRIPT_PATH=`dirname $0`

### Definition
GENEPI_ADDRESS="10.130.33.34"
GENEPI_USER="admin"
GENEPI_PWD="Manager01!"
GENEPI_ASSET="ups-13"
GENEPI_PUBLIC="public"
GENEPI_PRIVATE="private"
RETRY=5

### Usage of the command.
usage()
{
    echo ""
    echo "Usage: '$0' [options]"
    echo "Test genepi card"
    echo ""   
    echo "-h : show help"
    echo "-i <address> : set genepy card address"
    echo "-a <asset> : set genepy card asset"
    #echo "-u <user> : set the genepy card user"
    #echo "-p <password> : set the card password"
    echo ""
}


### Handle arguments
while getopts ":i:a:u:p:h" option; do
    case "$option" in
        i) GENEPI_ADDRESS=$OPTARG ;;
        a) GENEPI_ASSET=$OPTARG ;;
        u) GENEPI_USER=$OPTARG ;;
        p) GENEPI_PWD=$OPTARG ;;
        :) echo "missing argument";exit 1;;
        h) usage;exit 1 ;;
        \?) echo "unrecognized option or missing arg";;
    esac
done

nb_test_OK=0
nb_test_NOK=0

strindex() { 
  x="${1%%$2*}"
  [[ "$x" = "$1" ]] && return -1 || return ${#x}

#testob=`expr index "$1" $2` 

#echo $str | grep -b -o str | awk 'BEGIN {FS=":"}{print $1}'
#echo ${stringZ:1} 
}

nbstrfound() {
	chaine=$1
	res=1
	nb=0
	while [ $res -ne -1 ]; do
		#echo $chaine
		if [ ${#chaine} -ne 0 ]; then
			x="${chaine%%$2*}"
			[[ "$x" = "$chaine" ]] && res=-1 || res=${#x} 
			#echo "res=$res"
			if [ $res -ne -1 ]; then
			    length=${#2}
			    #echo "length=$length"
			    pos=$(( length + res ))
			    #echo "pos=$pos"
			    chaine=${chaine:$pos}
			    nb=$(( nb + 1 ))
			    #echo "nb=$nb"
			fi
        else
		    res=-1
        fi
    done
    return $nb
}

snmp_get() {
    res=$(snmpget -v 2c -c $GENEPI_PUBLIC $GENEPI_ADDRESS $1)
    #res=$?
    echo "res=$res"
    pos=`expr index "$res" :`
    #echo "pos=$pos"
    echo ${res:pos}
    return ${res:pos}
}

snmp_set() {
    res=$(snmpset -v 2c -c $GENEPI_PRIVATE $GENEPI_ADDRESS $1 i $2)    
    echo "res=$res"
}

# First close the COM port
#./HidSim close

# Set the device
#./HidSim setDevice 5P

# open the COM port
#./HidSim open
#STATUS=$?
#if [ $STATUS -ne 0 ]; then
#  cd - 1>/dev/null
#  echo "ci bench: error unable to open the COMPort of the HidSimD"
#  exit 1
#fi

# Init UPS objects
./HP1500_bat.sh

sleep 1

# ["Test index"]="SNMP OID:SNMP value to set:Status object:Status value:Alarm to search:Nb alarm 1:...:Alarm N to search:Nb alarm N
declare -A OBJ_SENSOR_LIST=(
    ["00"]=".1.3.6.1.4.1.534.6.8.1.2.2.1.5.NUM.1:350:ambient.NUM.temperature.status:warning-low:low temperature warning!:1"
    ["01"]=".1.3.6.1.4.1.534.6.8.1.2.2.1.6.NUM.1:340:ambient.NUM.temperature.status:critical-low:low temperature critical!:1"
    ["02"]=".1.3.6.1.4.1.534.6.8.1.2.2.1.6.NUM.1:0:ambient.NUM.temperature.status:warning-low:low temperature warning!:1"
    ["03"]=".1.3.6.1.4.1.534.6.8.1.2.2.1.5.NUM.1:100:ambient.NUM.temperature.status:good"

    ["04"]=".1.3.6.1.4.1.534.6.8.1.2.2.1.7.NUM.1:200:ambient.NUM.temperature.status:warning-high:high temperature warning!:1"
    ["05"]=".1.3.6.1.4.1.534.6.8.1.2.2.1.8.NUM.1:210:ambient.NUM.temperature.status:critical-high:high temperature critical!:1"
    ["06"]=".1.3.6.1.4.1.534.6.8.1.2.2.1.8.NUM.1:500:ambient.NUM.temperature.status:warning-high:high temperature warning!:1"
    ["07"]=".1.3.6.1.4.1.534.6.8.1.2.2.1.7.NUM.1:400:ambient.NUM.temperature.status:good"        

    ["08"]=".1.3.6.1.4.1.534.6.8.1.3.2.1.5.NUM.1:300:ambient.NUM.humidity.status:warning-low:low humidity warning!:1"
    ["09"]=".1.3.6.1.4.1.534.6.8.1.3.2.1.6.NUM.1:290:ambient.NUM.humidity.status:critical-low:low humidity critical!:1"
    ["10"]=".1.3.6.1.4.1.534.6.8.1.3.2.1.6.NUM.1:100:ambient.NUM.humidity.status:warning-low:low humidity warning!:1"
    ["11"]=".1.3.6.1.4.1.534.6.8.1.3.2.1.5.NUM.1:200:ambient.NUM.humidity.status:good"

    ["12"]=".1.3.6.1.4.1.534.6.8.1.3.2.1.7.NUM.1:210:ambient.NUM.humidity.status:warning-high:high humidity warning!:1"
    ["13"]=".1.3.6.1.4.1.534.6.8.1.3.2.1.8.NUM.1:220:ambient.NUM.humidity.status:critical-high:high humidity critical!:1"
    ["14"]=".1.3.6.1.4.1.534.6.8.1.3.2.1.8.NUM.1:900:ambient.NUM.humidity.status:warning-high:high humidity warning!:1"
    ["15"]=".1.3.6.1.4.1.534.6.8.1.3.2.1.7.NUM.1:800:ambient.NUM.humidity.status:good"        
)

snmp_get .1.3.6.1.4.1.534.6.8.1.1.1.0
nb_sensor=$?
echo "nb_sensor=$nb_sensor"
num_sensor=1
while [ $num_sensor -le $nb_sensor ]
do
    #for obj in ${!OBJ_SENSOR_LIST[@]}
    sorted=($(printf '%s\n' "${!OBJ_SENSOR_LIST[@]}"| /usr/bin/sort))
    for num_array in ${sorted[@]}    
    do
        echo "num_array=$num_array"
        IFS=":" read -r -a attribute <<< "${OBJ_SENSOR_LIST[$num_array]}"
        #echo "attribute=${attribute[0]} ${attribute[1]} ${attribute[2]} ${attribute[3]}"
        obj=${attribute[0]}
        obj_num=${obj/NUM/$num_sensor}    
        snmp_get $obj_num
        snmp_set $obj_num ${attribute[1]}
        #sleep 10
        # Test status
        status=${attribute[2]}
        status_num=${status/NUM/$num_sensor} 
        echo "$status_num=$status_num"                    
        status_value=$(upsc $GENEPI_ASSET $status_num)
        echo "$status_num=$status_value"        
        nbstrfound "$status_value" "${attribute[3]}"    
        if [ $? -eq 1 ]; then
            echo "Status OK for $status_num=${attribute[3]}"
            nb_test_OK=$(( nb_test_OK + 1 ))
        else
            echo "Status NOK!!!!!!!!!! for $status_num=${attribute[3]} -> Receive $status_value"
            nb_test_NOK=$(( nb_test_NOK + 1 ))
        fi
        # Test alarms
        echo "GENEPI_ASSET=$GENEPI_ASSET"
        alarm=$(upsc $GENEPI_ASSET ups.alarm)
        echo $alarm                
        index=4
        while [ 1 ]
        do
            alarm_name=${attribute[$index]}
            if [ -n "$alarm_name" ]; then
                index=$(( index + 1 ))
                alarm_nb=`echo ${attribute[$index]} | sed -e 's/ //g'`
                alarm_nb=$(($alarm_nb+0))
                index=$(( index + 1 ))
                echo "alarm_name=$alarm_name alarm_nb=$alarm_nb"
                nbstrfound "$alarm_name" "$alarm"                             
                #echo "res=$?"
                if [ $? -eq 1 ]; then
                    echo "Alarm $alarm_name:$alarm_nb OK"
                    nb_test_OK=$(( nb_test_OK + 1 ))                
                else                
                    echo "Alarm $alarm_name:$alarm_nb NOK!!!!!!!!!!!! -> $?"                
                    nb_test_NOK=$(( nb_test_NOK + 1 ))
                fi
            else
                break
            fi
        done
    done
    num_sensor=$(( num_sensor + 1 ))
done

unset sorted

# ["Object name"]="Value ON:Value OFF:Alarm to search:Status

    # Tested with HidSim simulator  
  # Tested with cdscmd (ssh + 9PX)  
# Not tested (Not present with cdscmd + 9PX)

# Not working: UPS.PowerConverter.Output.Overcurrent[1].PresentStatus.OverThreshold
# Not working: UPS.PowerSummary.PresentStatus.Good  -> Replace by UPS.PowerSummary.Status
declare -A OBJ_LIST=(
    ["UPS.PowerSummary.PresentStatus.ACPresent"]="0:1:On inverter!:OB"
    ["UPS.PowerSummary.PresentStatus.InternalFailure"]="1:0:Internal failure!:"
    ["UPS.PowerSummary.PresentStatus.OverTemperature"]="1:0:Bad temperature!:"
    ["UPS.PowerSummary.PresentStatus.CommunicationLost"]="1:0:Communication with UPS lost!:"
    ["UPS.PowerSummary.PresentStatus.FanFailure"]="1:0:Fan failure!:"
["UPS.PowerModule.PresentStatus.RedundancyLost"]="1:0:Redundancy lost!:"
["UPS.PowerModule.PresentStatus.CompatibilityFailure"]="1:0:Parallel or composite module failure!:"
    ["UPS.PowerConverter.Input[1].PresentStatus.Boost"]="1:0::OL BOOST"
    ["UPS.PowerConverter.Input[1].PresentStatus.Buck"]="1:0::OL TRIM"
["UPS.PowerConverter.Input[1].PresentStatus.CircuitBreaker"]="1:0:Breaker open!:"
  ["UPS.PowerConverter.Input[1].PresentStatus.VoltageOutOfRange"]="1:0:Input failure!:"
["UPS.PowerConverter.Inverter.PresentStatus.FuseFault"]="1:0:Fuse failure!:"
    ["UPS.PowerConverter.Input[1].PresentStatus.FuseFault"]="1:0:Fuse failure!:"
["UPS.PowerConverter.Rectifier.PresentStatus.FuseFault"]="1:0:Fuse failure!:"
["UPS.BatterySystem.Battery.PresentStatus.FuseFault"]="1:0:Fuse failure!:"
["UPS.System.Connector.Input.PresentStatus.Alarm"]="1:0:Building alarm!:"
["UPS.System.Slot[1].PresentStatus.Alarm"]="1:0:Building alarm!:"
  ["UPS.PowerConverter.Input[2].PresentStatus.Good"]="0:1:Bypass not available!:"
  ["UPS.PowerConverter.Input[2].PresentStatus.Used"]="1:0:On bypass!:BYPASS"  
  ["UPS.PowerConverter.Inverter.PresentStatus.InternalFailure"]="1:0:Powerswitch failure!:"
  ["UPS.PowerConverter.Chopper.PresentStatus.InternalFailure"]="1:0:Powerswitch failure!:"
    ["UPS.BatterySystem.Charger.PresentStatus.InternalFailure"]="1:0:Charger failure!:"
    ["UPS.PowerSummary.PresentStatus.Charging"]="1:0::CHRG"
    ["UPS.PowerSummary.PresentStatus.Discharging"]="1:0::DISCHRG"
    ["UPS.PowerSummary.PresentStatus.BelowRemainingCapacityLimit"]="1:0:Battery discharged!:LB" 
["UPS.BatterySystem.Battery.PresentStatus.FullyDischarged"]="1:0:Battery discharged!:LB"  
["UPS.BatterySystem.Battery.PresentStatus.Good"]="0:1:Battery bad!:RB"
    ["UPS.PowerSummary.PresentStatus.NeedReplacement"]="1:0:Battery bad!:RB"
    ["UPS.PowerConverter.Inverter.PresentStatus.InternalFailure"]="1:0:Inverter failure!:"  
    ["UPS.PowerSummary.PresentStatus.Overload"]="1:0:Output overload!:OVER"
  ["UPS.PowerConverter.Inverter.PresentStatus.Overload"]="1:0:Output overload!:OVER"
    ["UPS.PowerSummary.PresentStatus.ShutdownImminent"]="1:0:Shutdown imminent!:"
  ["UPS.PowerConverter.Output.Overcurrent[1].PresentStatus.OverThreshold"]="1:0:Bad output condition!:"
["UPS.PowerConverter.Output.PresentStatus.FrequencyOutOfRange"]="1:0:Bad output condition!:"  
["UPS.PowerConverter.Output.PresentStatus.VoltageTooHigh"]="1:0:Bad output condition!:"  
["UPS.PowerConverter.Output.PresentStatus.VoltageTooLow"]="1:0:Bad output condition!:"    
["UPS.PowerSummary.PresentStatus.ShutdownRequested"]="1:0:Output off as requested!:OFF"
["UPS.PowerConverter.Input[4].PresentStatus.Used"]="1:0:On maintenance bypass!:BYPASS"
    ["UPS.PowerSummary.PresentStatus.Good"]="0:1:Output off!:OFF"
    ["UPS.PowerSummary.Status"]="0:2:Output off!:OFF"    
    ["UPS.PowerSummary.PresentStatus.ShutdownRequested"]="1:0:Shutdown pending!:"
)

#for obj in ${!OBJ_LIST[@]}
sorted=($(printf '%s\n' "${!OBJ_LIST[@]}"| /usr/bin/sort))
for obj in ${sorted[@]}
do
    echo "obj=$obj"
    IFS=":" read -r -a attribute <<< "${OBJ_LIST[$obj]}"
    #echo "test=${OBJ_LIST[$obj]}"
    #echo "obj=$obj"
    echo "attribute=${attribute[0]} ${attribute[1]} ${attribute[2]} ${attribute[3]}"
    echo "Set $obj=${attribute[0]}"
    # Set alarm on
    echo "./HidSim set $obj ${attribute[0]}"
    #./HidSim set $obj ${attribute[0]}
    RES=$(./HidSim set $obj ${attribute[0]})
    echo "RES=$RES"
    nbstrfound "$RES" "Unknown command"    
    if [ $? -eq 0 ]; then
        # Test alarm present
        if [ -n "${attribute[2]}" ]; then
            i=1
            while [ $i -le $RETRY ]
            do
                sleep 1        
                alarm=$(upsc $GENEPI_ASSET ups.alarm)
                echo $alarm        
                nbstrfound "$alarm" "${attribute[2]}"    
                if [ $? -eq 1 ]; then
                    echo "Alarm ON OK for $obj"
                    nb_test_OK=$(( nb_test_OK + 1 ))
                    break
                else
                    if [ $i -eq $RETRY ]; then
                        echo "Alarm ON NOK!!!!!!!!! for $obj"
                        nb_test_NOK=$(( nb_test_NOK + 1 ))
                    fi  
                fi
                i=$(( i + 1 ))
            done
        fi

        # Test status present
        if [ -n "${attribute[3]}" ]; then
            i=1
            while [ $i -le $RETRY ]
            do
                sleep 1
                status=$(upsc $GENEPI_ASSET ups.status)
                echo $status        
                nbstrfound "$status" "${attribute[3]}"    
                if [ $? -eq 1 ]; then
                    echo "Status ON OK for $obj"
                    nb_test_OK=$(( nb_test_OK + 1 ))
                    break
                else
                    if [ $i -eq $RETRY ]; then
                        echo "Status ON NOK!!!!!!!!! for $obj"
                        nb_test_NOK=$(( nb_test_NOK + 1 ))
                    fi
                fi
                i=$(( i + 1 ))
            done
        fi

        echo "Set $obj=${attribute[1]}"
        # Set alarm off
        #./HidSim set $obj ${attribute[1]}
        RES=$(./HidSim set $obj ${attribute[1]})
        echo "RES=$RES"
        # Test alarm no present
        if [ -n "${attribute[2]}" ]; then
            i=1
            while [ $i -le $RETRY ]
            do
                sleep 1
                alarm=$(upsc $GENEPI_ASSET ups.alarm)
                echo $alarm        
                nbstrfound "$alarm" "${attribute[2]}"    
                if [ $? -eq 0 ]; then
                    echo "Alarm OFF OK for $obj"
                    nb_test_OK=$(( nb_test_OK + 1 ))
                    break
                else
                    if [ $i -eq $RETRY ]; then
                        echo "Alarm OFF NOK!!!!!!!!! for $obj"
                        nb_test_NOK=$(( nb_test_NOK + 1 ))
                    fi
                fi
                i=$(( i + 1 ))
            done
        fi
        # Test status not present
        if [ -n "${attribute[3]}" ]; then
            i=1
            while [ $i -le $RETRY ]
            do
                sleep 1
                status=$(upsc $GENEPI_ASSET ups.status)
                echo $status        
                nbstrfound "$status" "${attribute[3]}"    
                if [ $? -eq 0 ]; then
                    echo "Status OFF OK for $obj"
                    nb_test_OK=$(( nb_test_OK + 1 ))
                    break
                else
                    if [ $i -eq $RETRY ]; then
                        echo "Status OFF NOK!!!!!!!!! for $obj"
                        nb_test_NOK=$(( nb_test_NOK + 1 ))
                    fi
                fi
                i=$(( i + 1 ))
            done
        fi
    else 
        echo "$obj not implemented!!!!!!"
    fi
done

nb_test_TOTAL=$(( nb_test_OK + nb_test_NOK ))
if [ $nb_test_NOK -eq 0 ]; then
    echo ""
    echo ""
    echo " *************************************************************"
    echo "  All $nb_test_TOTAL tests passed"
    echo " *************************************************************"
    echo ""
else
    echo ""
    echo ""
    echo " *************************************************************"
    echo "  Only $nb_test_OK / $nb_test_TOTAL tests passed !!!!!!!!!!!!!"
    echo " *************************************************************"
fi
exit 0


