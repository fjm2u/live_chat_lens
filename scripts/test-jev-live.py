#!/usr/bin/env python3
"""Bounded live smoke: 4 sequential calls / 16 synthetic comments. macOS only.
Reads the explicitly saved Jev key from Keychain; never prints or writes it.
API charges may apply. Does not post anything to YouTube.
"""
import json
import pathlib
import subprocess
import sys
root = pathlib.Path(__file__).resolve().parents[1]
credential = subprocess.run(['security', 'find-generic-password', '-s', 'obs-comment-dock',
                             '-a', 'jev-api-key', '-w'], capture_output=True)
if credential.returncode:
    sys.exit('Could not read the saved Jev key from Keychain.')
results = []
for index in range(4):
    try:
        run = subprocess.run([str(root / 'build/jev-smoke')], input=credential.stdout,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=25)
    except subprocess.TimeoutExpired:
        results.append({'batch': index + 1, 'result': 'TIMEOUT'})
        break
    results.append({'batch': index + 1, 'returncode': run.returncode,
                    'output': run.stdout.decode('utf-8', errors='replace')})
    if run.returncode:
        break
for result in results:
    lines = result.get('output', '').splitlines()
    labels = {parts[0]: parts[1:] for line in lines if (parts := line.split()) and parts[0].startswith('synthetic-')}
    result['behavior_pass'] = (labels.get('synthetic-question', [])[:2] == ['show', 'high']
        and labels.get('synthetic-criticism', [])[:1] == ['show']
        and labels.get('synthetic-hostile', [])[:1] == ['hide']
        and labels.get('synthetic-greeting', [])[1:2] == ['low'])
report = {'mode': 'live_jev_synthetic_comments', 'requests': len(results), 'batches': results}
path = root / 'build/reports/jev-live.json'
path.parent.mkdir(parents=True, exist_ok=True)
path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(path.read_text(encoding='utf-8'))
sys.exit(0 if len(results) == 4 and all(r.get('returncode') == 0 and r.get('behavior_pass') for r in results) else 1)
