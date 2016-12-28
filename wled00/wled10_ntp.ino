void handleNetworkTime()
{
  if (ntpEnabled && ntpConnected)
  {
    if (ntpSyncNeeded)
    {
      if (ntpPacketSent)
      {
        if (getNtpTime())
        {
          ntpSyncNeeded = false;
          ntpPacketSent = false;
          ntpSyncTime = millis();
          Serial.print("Time: ");
          Serial.println(now());
        } else
        {
          if (millis() - ntpPacketSentTime > ntpRetryMs)
          {
            ntpPacketSent = false; //try new packet
          }
        }
      } else
      {
        WiFi.hostByName(ntpServerName, ntpIp);
        sendNTPpacket();
        ntpPacketSent = true;
        ntpPacketSentTime = millis();
      }
    } else if (millis() - ntpSyncTime > ntpResyncMs)
    {
      ntpSyncNeeded = true;
    }
  }
}

bool getNtpTime()
{
    int size = ntpUdp.parsePacket();
    if (size >= 48) {
        ntpUdp.read(ntpBuffer, 48);  // read packet into the buffer
        unsigned long secsSince1900;
        // convert four bytes starting at location 40 to a long integer
        secsSince1900 = (unsigned long)ntpBuffer[40] << 24;
        secsSince1900 |= (unsigned long)ntpBuffer[41] << 16;
        secsSince1900 |= (unsigned long)ntpBuffer[42] << 8;
        secsSince1900 |= (unsigned long)ntpBuffer[43];
        setTime(secsSince1900 - 2208988800UL + (millis() - ntpPacketSentTime)/2000); //naive approach to improve accuracy, utc
        return true;
    }
    return false; //unable to get the time
}

void sendNTPpacket()
{
    Serial.println("Sending NTP packet");
    memset(ntpBuffer, 0, 48);
    ntpBuffer[0] = 0b11100011;   // LI, Version, Mode
    ntpBuffer[1] = 0;     // Stratum, or type of clock
    ntpBuffer[2] = 6;     // Polling Interval
    ntpBuffer[3] = 0xEC;  // Peer Clock Precision
    ntpBuffer[12] = 49;
    ntpBuffer[13] = 0x4E;
    ntpBuffer[14] = 49;
    ntpBuffer[15] = 52;
    ntpUdp.beginPacket(ntpIp, 123); //NTP requests are to port 123
    ntpUdp.write(ntpBuffer, 48);
    ntpUdp.endPacket();
}

String getTimeString()
{
  local = CE.toLocal(now(), &tcr);
  String ret = monthStr(month(local));
  ret = ret + " ";
  ret = ret + day(local);
  ret = ret + " ";
  ret = ret + year(local);
  ret = ret + ", ";
  ret = ret + hour(local);
  ret = ret + ":";
  if (minute(local) < 10) ret = ret + "0";
  ret = ret + minute(local);
  ret = ret + ":";
  if (second(local) < 10) ret = ret + "0";
  ret = ret + second(local);
  return ret;
}
