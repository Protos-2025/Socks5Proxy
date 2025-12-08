#!/bin/bash
set -euo pipefail

# Run the build and propagate its exit code so the container exits non-zero
# when `make` fails. This makes compose's completion/depends_on behavior
# reflect the actual compile result.
make clean all
exit_code=$?
if [ "$exit_code" -ne 0 ]; then
	echo "Compile failed with exit code $exit_code" >&2
fi
exit $exit_code