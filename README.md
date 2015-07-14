
IDA WhatAPIs
=========================================================
IDA Pro plug-in to display contextual function API usage.
Version 1.1 April 2015
By Sirmabus

http://www.macromonkey.com/bb/index.php/topic,20.msg21.html#msg21
https://sourceforge.net/projects/idawhatapisplugin/


----- [Description] -----------------------------------------------------------
This plug-in will walk all of the functions in the loaded IDB and show you
what APIs are used inside each of these functions shown as a function comment.
The idea being to give you some helpful contextual information at a glance in
aid to reverse engineering.

----- [Install] ---------------------------------------------------------------
Copy the plug-in to your IDA Pro "plugins" directory.
Then edit your "..\plugins\plugins.cfg" to setup with a hotkey.

For example add these two lines:
; Sirmabus WhatAPIs plug-in
IDA_WhatAPIs_PlugIn.plw IDA_WhatAPIs_PlugIn.plw Ctrl-8 0

See IDA documentation for more on installing and using plug-ins.

----- [How to run it] ---------------------------------------------------------
Invoke like any other plug-in in IDA through the hot key, or via IDA's
Edit->Plugins menu.

Note: If you use my other contextual text comment plug-ins you should run them
in this exact order since some prepend, while others append their comments:
1) Function String Associate
2) Mark reference counts
3) WhatAPIs

Click "Continue" and let it run.

After it's done, browse around and you should now see appended function
API comments!

----- [Design] ----------------------------------------------------------------
Nothing too fancy, pretty simple, but maybe useful for others.
This is something that just came to me one day while spending many countless
hours reversing.

The APIs that a function might contain can be considered pretty significant.
The ability to see this infomation easier/quicker could be helpful.

----- [Change log] ------------------------------------------------------------
1.1 - 1) Updated SDK version.
      2) Updated custom wait dialog.

1.0 - Initial release.

Terms of Use
-------------------------------------------------------------------------------
This software is provided "as is", without any guarantee made as to its
suitability or fitness for any particular use. It may contain bugs, so use this
software is at your own risk.
The author(s) are not responsible for any damage that may be caused through its
use.
