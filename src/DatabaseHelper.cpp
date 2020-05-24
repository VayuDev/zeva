#include "DatabaseHelper.hpp"
#include "DatabaseWrapper.hpp"

namespace DatabaseHelper {

void attachNotifyTriggerToAllTables(DatabaseWrapper& pDb) {
    pDb.query(R"(
CREATE OR REPLACE FUNCTION table_changed() RETURNS trigger AS $BODY$
    BEGIN
       PERFORM pg_notify('onTableChanged', TG_TABLE_NAME || '%' || TG_OP);
       RETURN NEW;
    END;
    $BODY$ LANGUAGE plpgsql;
)");
    auto tables = pDb.query(R"(
SELECT table_name AS name
FROM information_schema.tables
WHERE table_schema = 'public'
)");
    for(size_t r = 0; r < tables->getRowCount(); ++r) {
        auto tablename = tables->getValue(r, 0).stringValue;

        pDb.query("DROP TRIGGER IF EXISTS notifyTrigger ON " + tablename);
        pDb.query("CREATE TRIGGER notifyTrigger AFTER INSERT OR UPDATE OR DELETE ON "+tablename+" EXECUTE FUNCTION table_changed()");
    }
}

}