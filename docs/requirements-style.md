# Requirements style — one language for the trackers

`sky-tracker`, `f1-tracker` and `plane-tracker` write their design documents the same way, so a
requirement is found, cited and amended the same way in all three. This is `sky-tracker`'s style.

## The document

```
# <Project> — Design Requirements (rev N)
<target hardware, framework, status, last updated>

## Changes in rev N        newest first; one line per version or change, citing the IDs
## 0. Context (CTX)
## 1. Architecture (ARCH)
## 2. Hardware constraints (HW)
## 3. Data (DATA)
## 4. Display (UI)
## 5. Motion (MOTION)
## 6. Performance and stability (PERF)
## 6a. Network (NET)
## 7. Build and deployment (BUILD)
## 8. Known failure modes (FAIL)
## 9. Verification (VER)
## 10. Non-goals (NG)
## Appendix A.. (optional)   a project's own: a decision log, open questions, milestones, ideas
```

An area a project has nothing to say about is left out or empty. A project may add an area prefix of
its own beside these (f1-tracker: `RACE` for the weekend state machine, `MAP` for circuit drawing,
`BOOT`), in the section where it belongs; sky-tracker adds `TEST`.

## A requirement

One requirement, one ID, one paragraph, written in the present tense:

```
UI-68  Internet updates (sky_update.h): ESPHome's http_request update entity reads
       manifest.json from GitHub Pages ... Only a NEWER version counts.
UI-68a The hourly check never opens anything by itself. ...
```

- **ID** = area prefix, a number, and for an amendment a lower-case letter after its parent
  (`UI-68a`, `UI-68b`). A letter refines or extends the parent; it never contradicts it. A
  requirement that is dropped keeps its ID in the log with the word *dropped*, and the number is
  not reused.
- The ID starts at column 0; continuation lines are indented to the text (seven spaces, or the
  ID's length plus two). Tables and code blocks go at column 0 under it.
- The paragraph says what MUST be so and why in a sentence (the symptom, the measurement); long
  rationale and measurements stay in the section's prose above the IDs.
- Code comments cite the ID (`// UI-68a`), not a section number. Section numbers may move; IDs do
  not.

## The log

`Changes in rev N` records what changed and when, newest first, one line per release or change:

```
- 4.6.40 (unreleased): the update prompt's release notes use the 16 px font (UI-68c).
- 4.6.39 (released): the constellation figures are drawn above the rings (UI-46d).
```

State is one of *unreleased*, *queued*, *installed*, *released*, or none. IDs, not section numbers,
are what the line cites.

A project that also keeps a numbered decision log (f1-tracker's Appendix A) keeps it as an appendix:
a decision records who asked, when, and what it supersedes; the requirements record what is now
true. A decision cites the IDs it creates or changes.

## Shared requirements

What `tracker-common` implements (the update screen, the web page patches, the About text, the
fonts, the glyph check) is specified once, in the project that first had it, and the others cite
that ID: f1-tracker's UI-68 is `sky-tracker`'s UI-68. When the shared behaviour changes, the change
is made in `tracker-common`, tagged, and each project's log notes the tag it moved to.
