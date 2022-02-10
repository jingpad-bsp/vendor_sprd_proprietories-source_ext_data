#!/system/bin/sh

LOG_TAG="data_rps.sh"
rfc=4096
cc=4
cpumask=f;

exec_cmd() {
	local cmd="$@"
	local emsg="$eval $cmd 2>&1"
}

function mlog()
{
	echo "$@"
	log -p d -t "$LOG_TAG" "$@"
}

function exe_log()
{
	mlog "$@";
	eval $@;
}

function rps_on()
{
	##Enable RPS (Receive Packet Steering)
	#cihipType=$(getprop ro.product.board)
	chipType=$(getprop ro.board.platform)
	if [ "$chipType" = "ud710" ]; then
		cc=8;
		cpumask=ff;
		netdev="sipa_eth";
	fi
	((rsfe=$cc*$rfc));
	echo "$rsfe";
	exe_log "echo $rsfe > /proc/sys/net/core/rps_sock_flow_entries"
	retVal=$(cat /proc/sys/net/core/rps_sock_flow_entries)
	mlog "the flow entries value is $retVal"
	for fileRps in $(ls /sys/class/net/$netdev*/queues/rx-*/rps_cpus)
		do
			exe_log "echo $cpumask > $fileRps";
			retVal=$(cat $fileRps)
			mlog "the value of $fileRps is $retVal"
		done
	for fileRfc in $(ls /sys/class/net/$netdev*/queues/rx-*/rps_flow_cnt)
		do
			exe_log "echo $rfc > $fileRfc";
			retVal=$(cat $fileRfc)
			mlog "the value of $fileRfc is $retVal"
		done
}

function rps_off()
{
	#chipType=$(getprop ro.product.board)
	chipType=$(getprop ro.board.platform)
	if [ "$chipType" = "ud710" ]; then
		netdev="sipa_eth";
	fi
	exe_log "echo 0 > /proc/sys/net/core/rps_sock_flow_entries"
	retVal=$(cat /proc/sys/net/core/rps_sock_flow_entries)
	mlog "the sock flow entries value is $retVal"
	for fileRps in $(ls /sys/class/net/$netdev*/queues/rx-*/rps_cpus)
		do
			exe_log "echo 0 > $fileRps";
			retVal=$(cat $fileRps)
			mlog "the value of $fileRps is $retVal"
		done
	for fileRfc in $(ls /sys/class/net/$netdev*/queues/rx-*/rps_flow_cnt)
		do
			exe_log "echo 0 > $fileRfc";
			retVal=$(cat $fileRfc)
			mlog "the value of $fileRfc is $retVal"
		done
}

function rps_roc()
{
	cpumask=ff;
	cc=8;
	netdev="sipa_eth";
	##Enable RPS (Receive Packet Steering)
	chipType=$(getprop ro.product.board)
	if [ "$1" = "1" ]; then
		cpumask=0f;
	fi
	((rsfe=$cc*$rfc));
	echo "$rsfe";
	exe_log "echo $rsfe > /proc/sys/net/core/rps_sock_flow_entries"
	retVal=$(cat /proc/sys/net/core/rps_sock_flow_entries)
	mlog "the flow entries value is $retVal"
	for fileRps in $(ls /sys/class/net/$netdev*/queues/rx-*/rps_cpus)
		do
			exe_log "echo $cpumask > $fileRps";
			retVal=$(cat $fileRps)
			mlog "the value of $fileRps is $retVal"
		done
	for fileRfc in $(ls /sys/class/net/$netdev*/queues/rx-*/rps_flow_cnt)
		do
			exe_log "echo $rfc > $fileRfc";
			retVal=$(cat $fileRfc)
			mlog "the value of $fileRfc is $retVal"
		done

	if [ -e "/proc/sys/net/core/rps_force_map" ]; then
		exe_log "echo 1 >/proc/sys/net/core/rps_force_map";
		retVal=$(cat "/proc/sys/net/core/rps_force_map")
		mlog "the value of rps_force_map is $retVal"
	fi
}

#for udx710_2h10 soc when com.anite.com apk's udp rate > 400Mb we run rps to make app run in big core
#lock freq to max and off Temperature control

function lockfreq_on()
{
	cc=8;
	netdev="sipa_eth";
	#if  rps_cpus value is“f0”,rps already on f0 we just break
	retVal=$(cat /sys/class/net/sipa_eth0/queues/rx-0/rps_cpus)
	if [ "$retVal" = "f0" ]; then
		exe_log "rps has been seted to 'f0'"
		exit 0
	fi
	 ##only rps_udp swith on ,lockfreq_on function go on
	rpsType=$(getprop persist.sys.rps.udp)
	if [ "$rpsType" = true ]; then
		mlog "rps_udp switch is on"
		cpumask=f0;
	else
		mlog "rps_udp swich is off"
		exit 0
	fi
	((rsfe=$cc*$rfc));
	echo "$rsfe";
	exe_log "echo $rsfe > /proc/sys/net/core/rps_sock_flow_entries"
	retVal=$(cat /proc/sys/net/core/rps_sock_flow_entries)
	mlog "the flow entries value is $retVal"
	for fileRps in $(ls /sys/class/net/$netdev*/queues/rx-*/rps_cpus)
		do
			exe_log "echo $cpumask > $fileRps";
			retVal=$(cat $fileRps)
			mlog "the value of $fileRps is $retVal"
		done
	for fileRfc in $(ls /sys/class/net/$netdev*/queues/rx-*/rps_flow_cnt)
		do
			exe_log "echo $rfc > $fileRfc";
			retVal=$(cat $fileRfc)
			mlog "the value of $fileRfc is $retVal"
		done
	if [ -e "/proc/sys/net/core/rps_force_map" ]; then
		exe_log "echo 1 > /proc/sys/net/core/rps_force_map";
		retVal=$(cat "/proc/sys/net/core/rps_force_map")
		mlog "the value of rps_force_map is $retVal"
	fi

	exe_log "echo 1 > /sys/kernel/debug/sipa_eth/gro_enable"
	exe_log "echo 0 > /sys/class/thermal/thermal_zone0/thm_enable"
	maxfreq=$(cat /sys/devices/system/cpu/cpufreq/policy4/cpuinfo_max_freq)
	exe_log "echo $maxfreq > /sys/devices/system/cpu/cpufreq/policy4/scaling_fix_freq"
	exec_cmd "/system/bin/iptables -D bw_global_alert 1"
	exec_cmd "/system/bin/iptables -D bw_INPUT 1"
	exec_cmd "/system/bin/iptables -D bw_OUTPUT 1"
	exec_cmd "/system/bin/iptables-save -t filter > /data/filter.log"
	exec_cmd "/system/bin/iptables -t filter -F"
}

function lockfreq_off()
{
	netdev="sipa_eth";
	##only rps_udp swith off and rate < 700M , lockfreq off
	rpsType=$(getprop persist.sys.rps.udp)
	if [ "$rpsType" = true ]; then
		mlog "rps_udp switch is on,rate < 700M"
		exit 0
	fi
	exe_log "echo 0 > /proc/sys/net/core/rps_sock_flow_entries"
	retVal=$(cat /proc/sys/net/core/rps_sock_flow_entries)
	mlog "the sock flow entries value is $retVal"
	for fileRps in $(ls /sys/class/net/$netdev*/queues/rx-*/rps_cpus)
		do
			exe_log "echo 0 > $fileRps";
			retVal=$(cat $fileRps)
			mlog "the value of $fileRps is $retVal"
		done
	for fileRfc in $(ls /sys/class/net/$netdev*/queues/rx-*/rps_flow_cnt)
		do
			exe_log "echo 0 > $fileRfc";
			retVal=$(cat $fileRfc)
			mlog "the value of $fileRfc is $retVal"
		done
	if [ -e "/proc/sys/net/core/rps_force_map" ];
	then
		exe_log "echo 0 >/proc/sys/net/core/rps_force_map";
		retVal=$(cat "/proc/sys/net/core/rps_force_map")
		mlog "the value of rps_force_map is $retVal"
	fi
	exe_log "echo 0 > /sys/kernel/debug/sipa_eth/gro_enable"
	exe_log "echo 1 > /sys/class/thermal/thermal_zone0/thm_enable"
	exe_log "echo 0 > /sys/devices/system/cpu/cpufreq/policy4/scaling_fix_freq"
	exec_cmd "/system/bin/iptables-restore < /data/filter.log"
}

if [ "$1" = "on" ]; then
	rps_on;
elif [ "$1" = "off" ]; then
	rps_off;
elif [ "$1" = "roc_m" ]; then
	rps_roc 1;
elif [ "$1" = "roc_h" ]; then
	rps_roc 2;
elif [ "$1" = "lockfreq_on" ]; then
	lockfreq_on;
elif [ "$1" = "lockfreq_off" ]; then
	lockfreq_off;
fi
