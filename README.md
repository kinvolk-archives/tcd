# tcd
traffic control daemon

## Try it out

* Download tcd with `go get github.com/kinvolk/tcd`
* Compile with `./build.sh`
* Install the D-Bus system policy with `sudo ./install.sh`
* Start `tcd` with `sudo ./bin/tcd`
* Start a rkt pod `sudo rkt run kinvolk.io/aci/busybox:1.24 --exec ping -- 8.8.8.8`
* Get the machine id with `machinectl`
* Install the network emulator queuing discipline and change its egress/ingress configuration
```bash
sudo gdbus call --system \
        --dest com.github.kinvolk.tcd \
        --object-path /com/github/kinvolk/tcd \
        --method com.github.kinvolk.tcd.Install \
        rkt-a6bd320a-2978-4789-90ae-233d5a221932
sudo gdbus call --system \
        --dest com.github.kinvolk.tcd \
        --object-path /com/github/kinvolk/tcd \
        --method com.github.kinvolk.tcd.ConfigureEgress \
        rkt-a6bd320a-2978-4789-90ae-233d5a221932 \
        10 0 800000 # latency: 10ms drop: 0% rate: 800000kbit
sudo gdbus call --system \
        --dest com.github.kinvolk.tcd \
        --object-path /com/github/kinvolk/tcd \
        --method com.github.kinvolk.tcd.ConfigureIngress \
        rkt-a6bd320a-2978-4789-90ae-233d5a221932 \
        15 50 800000 # latency: 15ms drop: 50% rate: 800000kbit
```
* Notice that it increases the latency by 25ms (10ms egress + 15ms ingress) and packet loss to 50%
* Call the D-Bus methods ConfigureIngress and ConfigureEgress again and notices that ping is modified immediately

