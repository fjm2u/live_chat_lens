# Release status

This repository is public for source review and development. **No stable binary release or OBS forum resource submission has been made.**

| Area | Current evidence | Before a stable download |
| --- | --- | --- |
| Source | MIT-licensed project source, tests, and documentation are public | Keep source aligned with each released binary and satisfy applicable OBS/third-party license terms |
| macOS arm64 | Local build loaded in OBS Studio 32.2.2; dock opened; core, protocol, and UI tests passed | Test an archive installed on a separate machine; sign and notarize a distributable package |
| YouTube → Jev → dock | Jev API smoke with synthetic input; no full live-chat acceptance | Verify a permitted live chat, API errors, reconnects, and expected display behavior |
| Streaming stability | Not measured | Exercise recording/streaming, long sessions, and frame-time impact |
| Windows / Linux | CMake targets exist; no real-device validation | Build, install, and test packages on each claimed platform |
| Privacy and security | Keys are intended to remain in memory; event logs omit message text | Review shipped artifacts and logs for credentials, personal data, and third-party dependencies |
| OBS forum listing | Not submitted | Meet the current [resource and IP policy](https://obsproject.com/forum/threads/forum-resource-and-ip-policy.178569/), including stable-release, source, branding, and description rules. AI use must be disclosed; resources written entirely or largely with AI coding tools are not permitted |

The development bundle in `build/` is ignored by Git. It is ad-hoc signed for local testing and is **not** a release artifact. A GitHub Release or OBS forum resource should point to a versioned, platform-specific archive that was tested after packaging. This checklist describes readiness; it does not claim that moderation will approve a submission.
