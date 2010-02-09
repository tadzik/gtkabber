Gtkabber
========

This is gtkabber. Just like it seems to be, it's an xmpp client in GTK+ library. It is also usable in single window, like TKabber. It's not yet finished, and not yet as cute as it is gonna be (scripting, remote-control, everything is still to be done), but I'm putting it here as there are some people willing to test it or even use it. If you are one of them, have fun. Also, I'm quite a begginer in C and espeially GTK+, so constructive criticism is really welcome.

1. Dependencies
---

Gtkabber depends on GTK+ (2.18 or newer), Loudmouth and Lua.

2. Configuration
---

The example.config file includes all the information needed

3. Usage
---

Adjust the config file for your needs and run 'main'. Everything _should_ work fine :) Some error messages might be printed in the status tab, if any of these worries you, send me your worries. You can now double-click buddies in your roster to start conversation with them (a new tab will appear on the right). If someone else starts conversation with you, a tab will also appear. You can use the following keybindings to speed up your actions:

* C-r - (Control-r) focus on roster, so you can use it without clicking the mouse
* C-t - focus on tabs
* C-h - show/hide unavailable buddies in roster
* C-s - focus on the status selection combobox
* C-M-s - (Control-Alt-s) focus on the status message entry field (press enter to set)
* C-q - close the current tab (this is actually the only way to close a tab :>)

Furthermore, you can use the following commands (enter them in the status tab's entry field):
* subscribe - display a popup bar to request someone's subsription. If the buddy is not in your roster, he/she will be added

3. What is alredy done
---

Chatting, setting your status and status message... that's all I'm afraid :) Still, you might trying it in your everday use (I'm the example of using it as an only xmpp client)

4. What is to be done
---

Some neat popular XEPs, push the lua scripting further. Any other ideas? Go on, share!

5. Reporting bugs
---

Please provide **as many information as possible**. Bugs that haven't yet been detected are likely to be hard to reproduce. When it comes to the roster view, this is especially important. Although seems stable, roster view seems to have funny reactions to some events (once you see it you will understand), though if anything like this happens to you, please provide me Everything you can think of. Loudmouth logs, from running gtkabber with LM_DEBUG="net" would be especially helpful.

Thanks for taking a look.

Ted
