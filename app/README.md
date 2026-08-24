# Tablet application

The existing .NET MAUI freETarget application is the functional UI reference.

For the 50 m project the transport layer will move from TCP/Wi-Fi to BLE GATT. The intention is to preserve scoring, target rendering and session functionality where practical while replacing connection/control services.

Initial BLE responsibilities:

- discover/pair/connect to target
- subscribe to SHOT and STATUS notifications
- write CONTROL / CONFIG commands
- configure and start rapid-fire sequences
- expose acoustic diagnostic data when requested
