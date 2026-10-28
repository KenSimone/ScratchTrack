This is a YNAB replacement I whipped up an eternity ago. Every now and then I try other options. It's October 2026 and I still don't really like the workflows of the alternatives.

Pros:

* Straight Win32 C++. Totally self contained. A single EXE is all you need. 
* The native file format is xlsx, and I spent a fair bit of time making the xlsx look good and work well. 
* No adds. No telemetry. 
* It's fast.
* It's Free.

Cons:
* Windows only
* No bank integration
* No cloud sync. Throw ScratchTrack.exe into a dropbox folder. There's your cloud sync. The data file is saved next to the exe.
* The importer is a limited. It works with the QIF exported from Chase bank.
* Reporting is dubious

Build with VS2022

Here's your documentation: https://youtu.be/UiiUf94v070?si=jQ4zf5lgE8r25imJ

I still use this daily but I've mostly stopped working on the code. If someone else wants to play with it for non-commercial use, go for it.

Third party libraries are OpenXSLX and zlib