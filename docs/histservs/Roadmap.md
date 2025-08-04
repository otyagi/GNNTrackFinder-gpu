Histo source: Tester binary = example of usage

- Members: hostname, port, ZMQ socket, list of known histo names

- Interface method/constructor: init socket\
  => Type? PUB or PUSH?\
  => PUSH
- Push config on startup, do not update afterward (just because enough for example, allowed in real lige)

- Message format
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Histo consummer: histserv

- Members: ZMQ socket
- Interface function: init socket\
  => Type? SUB or PULL?\
  => PULL
- Exec method: ZMQ poll/get loop\
  => Blocking?\
  => Exit condition?
- Config deserialization: boost?\
  => Example with it
- Histos deserialization: boost?\
  => Example with it
- Message processing: parts + calls to deserialization methods for Config and histo depending on size


TODO:
[x] Deserialization of configs in histserv
[x] Histograms filling in Tester
[x] Histos serialization in Tester
[x] Creation of configs in Tester
[x] Serialization of configs in Tester
[x] Multi-part message in Tester
[x] Test example: one periodic fill hist, one hist filled with "transmission time distribution", one canvas config
[ ] Signals from GUI to interact with server (Reset, Save, Stop) => no clue right now how to get them to work
[ ] Signals from CLI to have clean shutdown => Left to experts
[x] Documentation: to be done when structure is present after merging of the event builder MR
