# RaveloxMIDI

Please read **FAQ.md**

raveloxmidi is a simple proxy to send MIDI NoteOn, NoteOff, Control Change (CC) and Program Change (PC) events from the local machine via RTP MIDI or to receive any MIDI events from a remote source via RTP MIDI and write them to a file.

The reason for writing this was to generate note events from a Raspberry Pi to send them to Logic Pro X. In particular, using the Raspberry Pi to handle input from drum pads. As some people have started to use this, there have been several requests for the ability to send Control Change and Program Change events too. I've included some very basic python scripts for testing.

Thanks to feedback from a couple of users, I've also tested this with rtpMIDI on Windows talking to FL Studio 11.

The build will auto-detect ALSA and build rawmidi support. See below for ALSA requirements. Thanks to Daniel Collins (malacalypse) for being the guinea pig for this.

Except for the Avahi code, it's all mine but I have leaned heavily on the following references:

* RTP MIDI: An RTP Payload Format for MIDI ( http://www.eecs.berkeley.edu/~lazzaro/rtpmidi/index.html )
* The RTP MIDI dissector in wireshark. Written by Tobias Erichsen ( http://www.tobias-erichsen.de )

Note: Where possible, I've tried to use RTP MIDI to mean the protocol and rtpMIDI to mean the software written by Tobias Erichsen. Some mistakes may be present but,
in most cases, I am referring to the protocol

I'm doing this purely for fun and don't expect anyone else to use
it but I'm happy to accept suggestions if you ever come across
this code.

If you are using raveloxmidi, please drop me a line with some detail and I'll add your name to a list
with a link to your project.

## Description
There are a number of stages in the general RTP MIDI protocol, SIP, MIDI and feedback. The lazzaro document talks about SIP and feedback in terms of RTP but Apple has their own packet format for that information. raveloxmidi makes assumptions that may or may not be correct but the Apple MIDI SIP process isn't documented.

### Stage 1 - SIP
Audio MIDI setup needs to know which server to connect to. In order to do that, the raveloxmidi announces the Apple MIDI service (_apple-midi._udp). By default, the announcement is for connections on port 5004. This also implies port 5005 is open for connections too.

The connecting server will sent 2 connection (INV) requests containing its name, its IP address and the port that connections can be made on.

raveloxmidi will accept the first INV request and store the connection information in a table ( limited to 8 entries ) for later use.

raveloxmidi sends back an OK response to both requests.

The connecting server will send SYNC packets to raveloxmidi. According to http://en.wikipedia.org/wiki/RTP_MIDI, the sender will send timestamp1 to indicate its local time. This timestamp isn't an actual time value but can be a delta from a local value, the receiver will send timestamp2 with its local delta and then the sender will send timestamp3 indicating the time it received the timestamp2 response from the receiver. This gives the system an idea of latency.

SYNC packets are sent from the connecting server on a regular basis.

### Stage 2 - MIDI
This stage uses the defined MIDI RTP payload. raveloxmidi opens an additional listening port (by default 5006) which will accept a simple data packet containing note information. Where Apple MIDI packets are prefixed with a single byte 0xff, note data packets are expected to be prefixed with 0xaa. The rest of data packet is expected to be a MIDI Note or ControlChange command.

```
Command: 4 bits ( NoteOn = 0x09, NoteOff = 0x08 , ControlChange = 0x0B )
Channel: 4 bits ( 0 - 15 )
```

Note events are defined as:
```
Note: 8 bits ( -127 to 127 )
Velocity: 8 bits ( 0 to 127 )
```

Control Change events are defined as:
```
Controller Number: 8 bits (0 to 127 with 120-127 as Channel Mode messages)
Controller Value : 8 bits (0 to 127).
```

When raveloxmidi receives the command packet, it will send that MIDI command to any  active connections in the connection table. 

The RTP MIDI payload specification also requires a recovery journal ( see section 4 of RFC6295 at http://www.rfc-editor.org/rfc/rfc6295.txt ). raveloxmidi will add the note and control change events to the journal and attach the journal in each RTP packet sent to the connecting server. As raveloxmidi is only concerned with NoteOn, NoteOff and ControlChange events, only Chapter N and Chapter C journal entries are stored.

### Stage 3 - Feedback
The Apple MIDI implementation sends a feedback packet (RS) from the connecting server. This packet contains a RTP sequence number to indicate that the connecting server is acknowledging that it has received packets with a sequence number up to and including that particular value.

This tells the receiving server that it doesn't need to send journal events for any packets with a sequence number lower than that value.

raveloxmidi keeps track of the sequence number in the connection table and, if the value in the feedback packet is greater than or equal to the value in the table, the journal will be reset. 

## Inbound MIDI commands 
raveloxmidi will also accept inbound RTP-MIDI from remote hosts and will write the MIDI commands to a named file. MIDI commands are written at the time they are received and in the order that they are listed in the MIDI payload of the RTP packet. At this time, there is no handling of the RTP-MIDI journal on the inbound connection. A Feedback response is sent back when inbound midi events are received.

## Configuration
raveloxmidi can be run with a -c parameter to specify a configuration file with the options listed below.
Where the option isn't specified, a default value is used.

For debugging, you can run ```raveloxmidi -N -d``` to keep raveloxmidi in the foreground and send debug-level output to stderr.

### Options
```
network.bind_address
	IP address that raveloxmidi listens on. This can be an IPv4 or IPv6 address.
	Default is 0.0.0.0 ( meaning all interfaces ). IPv6 equivalent is ::
network.control.port
	Main RTP MIDI listening port for new connections and shutdowns.
	Used in the zeroconf definition for the RTP MIDI service.
	Default is 5004.
network.data.port
	Listening port for all other data in the conversation.
	Default is 5005.
network.local.port
	Local listening port for accepting MIDI events.
	Default is 5006.
network.max_connections
	Maximum number of incoming connections that can be stored.
	Default is 8.
service.name
	Name used in the zeroconf definition for the RTP MIDI service.
	Default is 'raveloxmidi'.
network.socket_timeout
	Polling timeout for the listening sockets.
	Default is 30 seconds.
run_as_daemon
	Specifies that raveloxmidi should run in the background.
	Default is yes.
daemon.pid_file
	If raveloxmidi is run in the background. The pid for the process is written to this file.
	Default is raveloxmidi.pid.
logging.enabled
	Set to yes to write output to a log file. Set to no to disable.
	Default is "yes".
logging.log_file
	Name of file to write logging to.
	Default is stderr.
logging.log_level
	Threshold for log events. Acceptable values are debug,info,normal,warning and error.
	Default is normal.
security.check
	If set to yes, it is not possible to write the daemon pid to a file with executable permissions.
	Default is yes.
inbound_midi
        Name of file to write inbound MIDI events to. This file is governed by the security check option.
	Default is /dev/sequencer
file_mode
        File permissions on the inbound_midi file if it needs to be created. Specify as Unix octal permissions. 
	Default is 0640.
```

If ALSA is detected, the following options are also available:

```
alsa.output_device
	Name of the rawmidi ALSA device to send MIDI events to.
alsa.input_device
	Name of the rawmidi ALSA device to read MIDI events from.
alsa.input_buffer_size
	Size of the buffer to use for reading data from the input device.
	Default is 4096. Maximum is 65535.
```

## ALSA Support

The autotools configure script will automatically detect the presence of ALSA libraries and will build the code for support.
raveloxmidi uses the rawmidi interface so the snd-virmidi module must be loaded.

The following steps can be taken to test everything is working:

1. Ensure the snd-virmidi module is loaded.

```modprobe snd-virmidi```

2. Verify the device names

```sudo amidi -l``` will give output like 
```
Dir Device Name
IO hw:0,0 ES1371
IO hw:1,0 Virtual Raw MIDI (16 subdevices)
IO hw:1,1 Virtual Raw MIDI (16 subdevices)
IO hw:1,2 Virtual Raw MIDI (16 subdevices)
IO hw:1,3 Virtual Raw MIDI (16 subdevices)
```

3. Install timidity and run it with the ALSA interface

```timidity -iA``` will output the available ports to connect to (for example):

```
Opening sequencer port: 128:0 128:1 128:2 128:3
```

4. In a raveloxmidi config file, add the option:

```alsa.output_device = hw:1,0,0```

The device name will vary depending on the setup.

5. Run raveloxmidi with the config file. In debug mode, the debug output should show lines like:

```
[1534193901]	DEBUG: raveloxmidi_alsa_init: ret=0 Success
[1534193901]	DEBUG: rawmidi: handle="hw:1,0,0" hw_id="VirMidi" hw_driver_name="Virtual Raw MIDI" flags=7 card=1 device=0
[1534193901]	DEBUG: rawmidi: handle="hw:1,0,0" hw_id="VirMidi" hw_driver_name="Virtual Raw MIDI" flags=7 card=1 device=0
```
6. Determine the port number for hw:1,0,0 using aconnect

```aconnect -l```

This will show output like:
```
client 0: 'System' [type=kernel]
0 'Timer '
1 'Announce '
client 14: 'Midi Through' [type=kernel]
0 'Midi Through Port-0'
client 16: 'Ensoniq AudioPCI' [type=kernel,card=0]
0 'ES1371 '
client 20: 'Virtual Raw MIDI 1-0' [type=kernel,card=1]
0 'VirMIDI 1-0 '
```

This shows that ```hw:1,0,0``` is port ```20:0```

7. Connected the port to timidty:

```aconnect 20:0 128:0```

8. On the remote machine, make a connection to raveloxmidi. I have tested this with OS X.
9. (For example) In Logic Pro X, create a new external MIDI track and use the raveloxmidi connection.
10. Using the keyboard GUI in Logic Pro X, tap a few notes. The notes are played through Timidity.


For input support:

1. Repeat steps 1 and 2 above if the module isn't loaded.

2. In a raveloxmidi config file, add the option:

```alsa.input_device = hw:1,1,0```

The device name will vary depending on the setup but it MUST be different from the device configuired as the output device.

3. Run raveloxmidi with the config file. In debug mode, the debug output should show lines like:
```
[1534193901]    DEBUG: raveloxmidi_alsa_init: ret=0 Success 
[1534193901]    DEBUG: rawmidi: handle="hw:1,0,0" hw_id="VirMidi" hw_driver_name="Virtual Raw MIDI" flags=7 card=1 device=0
[1534193901]    DEBUG: rawmidi: handle="hw:1,1,0" hw_id="VirMidi" hw_driver_name="Virtual Raw MIDI" flags=7 card=1 device=0
```
4. Determine the port number for ```hw:1,1,0``` using aconnect

```aconnect -l``` will show output like:
```
client 0: 'System' [type=kernel]
0 'Timer '
1 'Announce '
client 14: 'Midi Through' [type=kernel]
0 'Midi Through Port-0' 
client 16: 'Ensoniq AudioPCI' [type=kernel,card=0]
0 'ES1371 '
client 20: 'Virtual Raw MIDI 1-0' [type=kernel,card=1]
0 'VirMIDI 1-0 '
client 21: 'Virtual Raw MIDI 1-0' [type=kernel,card=1]
0 'VirMIDI 1-0 '
```
This shows that ```hw:1,1,0``` is port ```21:0```

5. On the remote machine make a connection raveloxmidi.
6. Run your favourite music making software that will make MIDI connections.
7. On the local machine, Using aplaymidi, take a .mid file and run:

```aplaymidi -p 21:0 name-of-midi-file.mid```

The MIDI events should be processed through the remote software.
