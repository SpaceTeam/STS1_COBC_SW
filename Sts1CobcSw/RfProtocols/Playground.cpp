auto Version1() -> void
{
    auto message = pus::Ack();
    auto message1 = pus::Nack(ErrorCode::io);
    auto report = pus::ParameterReport("txBaudRate", 115200);
    // Alternatively, name it something like Build...Message/Report(), and maybe we don't need pus::

    auto apid = 3;
    // I am not yet sure who determines/knows if there is a secondary header or not
    auto packet = SpacePacket(apid, Span(Serialize(message)));
    // or if message is just a byte array (or vector) so there is no need to serialize it
    auto packet1 = SpacePacket(apid, message);
    // Alternatively, name it spp::Packet(), spp::Wrap() or WrapInSpacePacket()

    auto frame = TmTransferFrame();
    // Maybe we don't need this and a global default is enough
    frame.SetIdlePackageGenerator(/*generator*/);
    auto vcid = 1;
    frame.StartNewFrame(vcid);
    // I am not sure what we should require from packet. It might jsut be a byte array (or vector)
    // or it's any type that support SerializeTo() (normal Serialize() is not possible since packets
    // have dynamic size)
    frame.Add(packet);
    frame.Add(packet1);
    frame.Finish();  // Appends an idle package and filles out the header
}
