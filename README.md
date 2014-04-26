NetSim
======

NetSim is a wireless network simulator. NetSim is primarily designed for use
by developers of the network stacks. It can also be used for initial design,
development and debugging of the final applications, but its use in this
capacity may be limited because of the way simulated peripherals are designed.

Simulated devices implement a wireless system on a chip (SoC) based on
an ARMv6-M-compatible MCU (Cortex-M0) and an abstract IEEE 802.15.4 radio
transceiver. By default simulated system runs at 1 MHz, which is usually fast
enough for a networking stack and a simple application.

NetSim uses pseudorandom number generator with random, but repeatable output.
This allows to run the same simulation over and over with predictable results.

## Installation

NetSim comes with a very simple `Makefile`. To build NetSim simply run

    make clean all

in the `netsim` directory. 

By default NetSim is configured to use internal version of the ARMv6-M core.
To enable a bit faster version of the core, set 

    USE_LIBCORE = 1

in the `Makefile`.

You will have to build libcore separately. To do this, run

    make gen
    make all

in the `netsim/libcore` directory. After build is done, simply compile the main
application again.

Note that libcore build requires a lot of RAM and might take a while.

## Running

NetSim is a command line application. A name of the configuration file
describing a simulated network must be provided as an argument. For example,

    $ netsim PingPong_4x4.cfg

By default NetSim is configured to output debugging information from the
simulated transcivers. This allows for monitoring of the simulation status, but
on slower trminals this may slow down simulation. To disable debug output, set

    #define DEBUG_TRX          0

in the `main.h` file and recompile NetSim.

## Network Configuration

Network configuration is described in a plain text file. Each line of the file
contains a declaration for a single network element. Lines starting with `#` are
treated as comments and ignored during processing.

Configuration file may include any combination of the commands described
below.

Integer numbers may be written in decimal or hexadecimal format. Floating point
numbers may be written in natural or scientific notation. Strings must be written
as is, without quotes and should not contain spaces.

### Random Seed

This command defines the seed value for the pseudorandom number generator used
for generating all random numbers during the simulation.

`seed` must be an integer in the rage 0-4294967296 (unsigned 32 bit).

Format:

    seed	<seed>

 * seed -- seed value for the pseudorandom number generator

Example:

    seed	123456

### Simulation Time

This command defines the simulation time.

`time` must be an integer in the range 0-2^64 (unsigned 64 bit).

Format:

    time	<time>

 * time -- simulation time (microseconds)

Example:

    time	10000000

### Coordinates Scale

This command defines a scaling factor applied to all coordinates defined
after the `scale` command is used. This allows for easy scaling of the
physical network dimensions without updating all coordinates.

`scale` command may be used more than once. Scaling is applied to all the
following declarations until a new `scale` command is used or until
the end of the file is reached. The default scaling factor is `1.0`.

Format:

    scale	<scale>

 * scale -- scaling factor applied to all following coordinates

Example:

    scale	2.5

### Node

This command defines a node (SoC) located at the coordinates (`x`, `y`).
Simulated program memory of the MCU is initialized with the contents of
the `firmware` image. Firmware image must be in a raw binary format.

The `id` parameter is an unsigned 32-bit integer that is directly available
to the application running on the MCU. This allows for the same firmware
running on different nodes behave differently. For example this parameter
might represent a network address of the node.

Format:

    node	<name> <x> <y> <id> <firmware>

 * name -- name of the node (used for logging)
 * x -- X coordinate of the transceiver (meters)
 * y -- Y coordinate of the transceiver (meters)
 * id -- ID of the node
 * firmware -- name of the MCU firmware

Example:

    node	R_0	-10.0	50.0	123	PingPong.bin

### Sniffer

This command defines a sniffer located at the coordinates (`x`, `y`). 

The sniffer is capable of receiving frames on a single channel or a range of
channels. The `frequency` parameter must be a single integer to define
a single frequency, and two integers separated by a `-` character to define
a range of frequencies.

`sensitivity` of the sniffer defines an effective radius at which sniffer
can receive frames. Wile having an infinite sensitivity would be a distinct
advantage of the simulated sniffer compared to a real one, having all frames
received from the entire network can be confusing and hard to analyze.
This is especially true for large networks, where parts of the network
are physically separated and can not reach each other parts directly.

Currently sensitivity of the simulated radio is `-100 dBm`, so keeping
sniffer at the same sensitivity will simulate a sniffer receiver comparable
to the receiver in the node's transceiver.

More than one sniffer may be defined at the same time. This may simplify
analysis of the distributed networks.

Sniffer logs are saved in the Daintree Networks SNA format (*.dcf). This
format is widely used in the industry and can be opened by many free and
proprietary sniffers.

Format:

    sniffer	<name> <x> <y> <frequency> <sensitivity> <output>

 * name -- name of the sniffer (used for logging)
 * x -- X coordinate of the sniffer (meters)
 * y -- Y coordinate of the sniffer (meters)
 * frequency -- frequency or frequency range (MHz)
 * sensitivity -- receiver sensitivity (dBm)
 * output -- name of the sniffer log output file

Example:

    sniffer	S_0	0.0	0.0	2405-2480	-100.0	sniffer.dcf

### Noise Source

This command defines a noise source located at the coordinates (`x`, `y`). 

The noise source is capable of generating an uncorrelated white noise on
a single channel or a range of channels. The `frequency` parameter must
be a single integer to define a single frequency, and two integers
separated by a `-` character to define a range of frequencies.

Power of the generated noise is defined by the `power` parameter.

`on_time` and `off_time` define timing parameters of the generated noise.
If `off_time` parameter is `0`, then noise is generated constantly and the
value of the `on_time` parameter does not matter.

Format:

    noise	<name> <x> <y> <frequency> <power> <on_time> <off_time>

 * name -- name of the noise source (used for logging)
 * x -- X coordinate of the noise source (meters)
 * y -- Y coordinate of the noise source (meters)
 * frequency -- frequency or frequency range (MHz)
 * power -- power of the noise source (dBm)
 * on_time -- active duration (microseconds)
 * off_time -- inactive inactive (microseconds)

Example:

    noise	N_1	100.0	100.0	2405-2480	-10.0	2000000	1000000


