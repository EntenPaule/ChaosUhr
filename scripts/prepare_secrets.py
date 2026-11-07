"""Create per-project credentials once, without printing them in build logs."""
import os
from pathlib import Path
import secrets

Import("env")

secret_path = Path(env.subst("$PROJECT_DIR")) / "include" / "secrets.local.h"
secret_path.parent.mkdir(parents=True, exist_ok=True)
try:
    descriptor = os.open(secret_path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
except FileExistsError:
    pass  # Keep the same credentials across firmware updates.
else:
    with os.fdopen(descriptor, "w", encoding="utf-8") as output:
        output.write(
            "// Local credentials. Do not commit or share this file.\n"
            "#pragma once\n"
            f'#define CHAOSUHR_OTA_PASSWORD "{secrets.token_urlsafe(24)}"\n'
            f'#define CHAOSUHR_SETUP_PASSWORD "{secrets.token_urlsafe(18)}"\n'
        )
    print("Created include/secrets.local.h with local Web/OTA and setup credentials.")
