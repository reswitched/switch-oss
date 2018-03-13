/*--------------------------------------------------------------------------------*
  Copyright (C)Nintendo All rights reserved.
 
  These coded instructions, statements, and computer programs contain proprietary
  information of Nintendo and/or its licensed developers and are protected by
  national and international copyright laws. They may not be disclosed to third
  parties or copied or duplicated in any form, in whole or in part, without the
  prior written consent of Nintendo.
 
  The content herein is highly confidential and should be handled accordingly.
 *--------------------------------------------------------------------------------*/
void sdb_customizeDB(sqlite3 *sqlDB) {
    /*  On the Nintendo platform, journaling is not needed as the
        backing fs is journaled and the store(s) used by SSL are
        completed wiped each boot.

        Other pragmas which are available but not shown to have a
        significant impact in performance are:

        temp_store=memory    Use memory for temp storage rather than fs
        synchronous=off      Do not force sync of the fs after updates
        count_changes=off    Do not count row changes on insert, update
    */
    sqlite3_exec(sqlDB, "PRAGMA journal_mode=off", NULL, 0, NULL);
}
