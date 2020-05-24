#pragma once

class DatabaseWrapper;

namespace DatabaseHelper {
    void attachNotifyTriggerToAllTables(DatabaseWrapper& pDb);
}