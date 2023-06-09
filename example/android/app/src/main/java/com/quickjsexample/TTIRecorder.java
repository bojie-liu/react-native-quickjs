//*** licence placeholder ***//

package com.quickjsexample;

import com.facebook.common.logging.FLog;

class TTIRecorder {
   static public long mOnCreateTimestamp;
   static public long mTTI;
   static public long mComponentDidMount;

   static public void printTTI() {
      if (mTTI != 0 && mComponentDidMount != 0) {
         FLog.e("TTIRecorder", "TTI: " + mTTI + " : " + "ComponentDidMount: " + mComponentDidMount);
      }
   }
}
