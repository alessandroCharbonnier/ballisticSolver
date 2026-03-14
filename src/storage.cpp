#include "storage.h"
#include "config.h"
#include <Preferences.h>

// NVS keys
static const char* KEY_RIFLE    = "rifle";
static const char* KEY_STAGES   = "stages";
static const char* KEY_STAGE_N  = "stage_n";

bool Storage::load(RifleConfig& rifle, StageConfig& stages) {
    Preferences prefs;
    prefs.begin(cfg::NVS_NAMESPACE, true);  // read-only

    // Check if config exists
    if (!prefs.isKey(KEY_RIFLE)) {
        prefs.end();
        return false;
    }

    // Load rifle config as binary blob
    size_t len = prefs.getBytes(KEY_RIFLE, &rifle, sizeof(RifleConfig));
    if (len != sizeof(RifleConfig)) {
        // Size mismatch (firmware update changed struct) — use defaults
        rifle = RifleConfig{};
    }

    // Load stage count
    stages.count = prefs.getUChar(KEY_STAGE_N, 0);
    if (stages.count > MAX_STAGES) stages.count = MAX_STAGES;

    // Load each stage target
    if (stages.count > 0) {
        len = prefs.getBytes(KEY_STAGES, &stages.targets,
                             sizeof(StageTarget) * stages.count);
        if (len != sizeof(StageTarget) * stages.count) {
            stages.count = 0;
        }
    }

    prefs.end();
    return true;
}

void Storage::save(const RifleConfig& rifle, const StageConfig& stages) {
    Preferences prefs;
    prefs.begin(cfg::NVS_NAMESPACE, false);  // read-write

    prefs.putBytes(KEY_RIFLE, &rifle, sizeof(RifleConfig));
    prefs.putUChar(KEY_STAGE_N, stages.count);

    if (stages.count > 0) {
        prefs.putBytes(KEY_STAGES, &stages.targets,
                       sizeof(StageTarget) * stages.count);
    }

    prefs.end();
}

void Storage::erase() {
    Preferences prefs;
    prefs.begin(cfg::NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
}
