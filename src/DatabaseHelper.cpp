#include "DatabaseHelper.hpp"
#include "DatabaseWrapper.hpp"

namespace DatabaseHelper {

void attachNotifyTriggerToAllTables(DatabaseWrapper &pDb) {
  pDb.query(R"(
CREATE OR REPLACE FUNCTION table_changed() RETURNS trigger AS $BODY$
    BEGIN
       PERFORM pg_notify('on_table_changed', TG_TABLE_NAME || '%' || TG_OP);
       RETURN NEW;
    END;
    $BODY$ LANGUAGE plpgsql;
)");
  auto tables = pDb.query(R"(
SELECT table_name AS name
FROM information_schema.tables
WHERE table_schema = 'public'
)");
  for (size_t r = 0; r < tables->getRowCount(); ++r) {
    auto tablename = tables->getValue(r, 0).stringValue;

    pDb.query("DROP TRIGGER IF EXISTS notifyTrigger ON " + tablename);
    pDb.query(
        "CREATE TRIGGER notifyTrigger AFTER INSERT OR UPDATE OR DELETE ON " +
        tablename + " EXECUTE FUNCTION table_changed()");
  }
}

void createDb(DatabaseWrapper &pDb) {

  pDb.query("CREATE TABLE IF NOT EXISTS scripts (id BIGSERIAL PRIMARY KEY, "
            "name TEXT UNIQUE, code TEXT)");
  try {
    pDb.query(
        "CREATE TABLE protected (id BIGSERIAL PRIMARY KEY, name TEXT UNIQUE)");
    pDb.query(R"(
INSERT INTO protected (name) VALUES
    ('scripts'),
    ('protected'),
    ('log'),
    ('log_level'),
    ('timelog'),
    ('timelog_activity'),
    ('timelog_entry')
)");
  } catch (...) {
  }
  // timelog table
  pDb.query(R"(
CREATE TABLE IF NOT EXISTS timelog (
    id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL
))");
  pDb.query(R"(
CREATE TABLE IF NOT EXISTS timelog_activity (
    id BIGSERIAL PRIMARY KEY,
    timelogid BIGINT NOT NULL REFERENCES timelog(id) ON DELETE CASCADE,
    name TEXT NOT NULL
))");
  pDb.query(
      R"(
CREATE TABLE IF NOT EXISTS timelog_entry (
    id BIGSERIAL PRIMARY KEY,
    timelogid BIGINT NOT NULL REFERENCES timelog(id) ON DELETE CASCADE,
    activityid BIGINT NOT NULL REFERENCES timelog_activity(id) ON DELETE CASCADE,
    created TIMESTAMP WITH TIME ZONE NOT NULL,
    duration INTERVAL
))");
  // log table
  try {
    pDb.query(
        "CREATE TABLE log_level (id BIGSERIAL PRIMARY KEY, name TEXT UNIQUE)");
    pDb.query(R"(
INSERT INTO log_level (id, name) VALUES
    (0, 'trace'),
    (1, 'debug'),
    (2, 'info'),
    (3, 'warn'),
    (4, 'error'),
    (5, 'fatal'),
    (6, 'syserr');
)");
  } catch (std::exception &e) {
  }

  pDb.query(R"(
CREATE TABLE IF NOT EXISTS log (
    id BIGSERIAL PRIMARY KEY,
    created TIMESTAMP WITH TIME ZONE NOT NULL,
    level BIGINT NOT NULL REFERENCES log_level(id),
    msg TEXT NOT NULL
))");
  // log trigger
  pDb.query(R"(
CREATE OR REPLACE FUNCTION new_log() RETURNS trigger AS $BODY$
    BEGIN
        IF TG_OP = 'INSERT' THEN
          PERFORM pg_notify('newlog', TG_OP || '%' || NEW.id :: TEXT);
        ELSE
          PERFORM pg_notify('newlog', TG_OP || '%');
        END IF;
        RETURN NULL;
    END;
    $BODY$ LANGUAGE plpgsql;
)");
  try {
    pDb.query("DROP TRIGGER newLogTrigger ON log");
  } catch (...) {
  }
  try {
    pDb.query("CREATE TRIGGER newLogTrigger AFTER INSERT OR UPDATE OR DELETE "
              "ON log FOR EACH ROW "
              "EXECUTE FUNCTION new_log()");
  } catch (...) {
  }
  // system monitor
  pDb.query(R"(
CREATE TABLE IF NOT EXISTS system_monitor_device (
  id BIGSERIAL PRIMARY KEY,
  name TEXT NOT NULL UNIQUE
))");
  pDb.query(R"(
CREATE TABLE IF NOT EXISTS system_monitor_entry (
  id BIGSERIAL PRIMARY KEY,
  deviceid BIGINT NOT NULL REFERENCES system_monitor_device(id) ON DELETE CASCADE,
  time TIMESTAMP WITHOUT TIME ZONE,
  total_cpu_usage REAL,
  cpu_core_count INT,
  uptime BIGINT,
  load_1 REAL,
  load_5 REAL,
  load_15 REAL,
  total_ram INT,
  free_ram INT,
  shared_ram INT,
  buffer_ram INT,
  total_swap INT,
  free_swap INT,
  proc_count INT,
  total_high INT,
  free_high INT
))");
  DatabaseHelper::attachNotifyTriggerToAllTables(pDb);
}

} // namespace DatabaseHelper