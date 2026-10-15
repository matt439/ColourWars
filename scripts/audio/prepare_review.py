"""Assemble the POL#22 catalogue, provenance and reproducible offline audition.

python scripts/audio/prepare_review.py
Requires numpy and ffmpeg. Render the three production projects first.
"""
from __future__ import annotations

import hashlib
import html
import json
from pathlib import Path
import subprocess
import wave

import numpy as np

from audio_pipeline import loop_region, riff_chunks

ROOT = Path(__file__).resolve().parents[2]
PRODUCTION = ROOT / "game/audio/paint_exe"
RATE = 44100
REQUIRED = """
music.menu music.close_quarters music.king_of_the_hill music.turbulence music.results music.end_sting
match.count_beat match.go match.time_up match.glass_out match.glass_in match.reveal_hover
match.reveal_click match.reveal_close match.celebrate match.slump match.colour_in match.skip
match.undo match.last_ten room.sigh room.chair room.tone room.footsteps room.breath match.waver match.cheat
player.jump player.land player.step player.skid player.hurt player.pop player.redraw player.respawn
player.step_painted player.bonk player.drop player.hit_confirm
sprayer.loop sprayer.start sprayer.stop roller.loop roller.start roller.stop roller.lift roller.settle
weapon.empty weapon.recovered mister.loop mister.start mister.stop sniper.shot sniper.ready bucket.throw
paint.splash paint.capture paint.dud paint.impact.sprayer paint.impact.roller paint.impact.mister
paint.impact.sniper paint.impact.bucket weapon.blocked bucket.slosh tool.drip
world.ambience.close_quarters world.ambience.king_of_the_hill world.ambience.turbulence world.detail
ui.move ui.confirm ui.cancel ui.error ui.ready ui.pause_open ui.unready ui.pad_lost ui.pad_back
ui.count_in results.fill results.winner results.draw end.series ui.volume_preview
""".split()


def save_json(path: Path, value: dict) -> None:
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def acceptance_status(cues: list[dict]) -> str:
    path = PRODUCTION / "acceptance.json"
    if path.is_file():
        record = json.loads(path.read_text(encoding="utf-8"))
        masters = {cue["id"]: cue["sha256"] for cue in cues}
        if (record.get("status") == "accepted" and len(masters) == len(cues)
                and record.get("master_sha256") == masters):
            return f"Author accepted this set on {record['date']} for POL#23 integration."
    return "Production candidates; listening acceptance pending."


def collect() -> tuple[list[dict], dict[str, dict]]:
    cues, sources = [], {}
    for name in ("music", "foley", "interface"):
        part = PRODUCTION / name
        for source in json.loads((part / "sources.json").read_text(encoding="utf-8"))["sources"]:
            if source["id"] in sources:
                raise ValueError(f"Duplicate source id: {source['id']}")
            sources[source["id"]] = source
        for cue in json.loads((part / "cues.json").read_text(encoding="utf-8"))["cues"]:
            cue = dict(cue)
            cue.setdefault("cue_id", cue["id"])
            path = ROOT / cue["path"]
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            if cue.get("sha256", digest) != digest:
                raise ValueError(f"Master hash differs from its project registry: {cue['id']}")
            cue["sha256"] = digest
            # Browser audio.loop and the offline mix repeat the whole file.
            # Refuse a partial smpl region rather than review a different join.
            if cue["loop"]:
                with wave.open(str(path), "rb") as source:
                    frames = source.getnframes()
                if loop_region(riff_chunks(path.read_bytes()), frames) != (0, frames):
                    raise ValueError(f"Review requires a full-file loop: {cue['id']}")
            cues.append(cue)
    cues.sort(key=lambda cue: (cue["tier"], cue["id"]))
    save_json(PRODUCTION / "cues.json", {
        "version": 1, "status": acceptance_status(cues),
        "required_cues": REQUIRED, "cues": cues,
    })
    save_json(PRODUCTION / "sources.json", {"version": 1, "sources": list(sources.values())})
    save_json(PRODUCTION / "paint_exe.json", {
        "waves": [cue["id"] for cue in cues], "create_effect_instance_for_each_wave": False,
        "sound_effect_instances": [],
    })
    return cues, sources


def provenance(cues: list[dict], sources: dict[str, dict]) -> None:
    status = acceptance_status(cues)
    lines = ["# Paint.exe audio provenance", "",
        "Produced 2026-10-14 for POL#22. Masters stay outside live content; the runtime bank uses accepted exports.",
        status + " Runtime mix acceptance remains ROAD#3.",
        "[acceptance.json](acceptance.json) records the author's response and the exact accepted master hashes.",
        "Every source is retained and hashed in [sources.json](sources.json). Every exported wave",
        "has the record below and a hash in [cues.json](cues.json). No legacy audio was used.", "",
        "The source recordings use CC0 1.0. Source redistribution and bundling in a release/bank",
        "are permitted by each source's recorded licence. No attribution is required; voluntary",
        "credits are retained here and in NOTICE. Original ColourWars arrangements and edit recipes",
        "were authored with Codex; repository-original production code and arrangements use the",
        "repository MIT licence. Source recordings retain their CC0 terms. No human recording",
        "by the repository author is claimed.", "",
        "Exports: PCM signed 16-bit, 44.1 kHz; music stereo, effects mono except room/ambient beds.",
        "The bank build encodes music as MS-ADPCM (512-frame, 524-byte stereo blocks); full-file",
        "loops add at most 511 derived boundary frames. Effects remain PCM. Runtime export hashes,",
        "padding and decoded checks are in scripts/audio/runtime-bank.json. Each project retains source excerpts, pitches,",
        "layer timings, filters, gains, fades and loop edits in its recipe and renderer.", "",
        "## Recorded sources", ""]
    for sid, source in sorted(sources.items()):
        source_path = Path(source["path"]).relative_to(PRODUCTION.relative_to(ROOT)).as_posix()
        lines += [f"### {sid}", "", f"- Creator: {source['creator']}.",
            f"- Source: [{source.get('archive_member', sid)}]({source['url']}).",
            f"- Licence: [{source['license']}]({source['license_url']}).",
            f"- Release/bank use: yes. Editable-source redistribution: yes.",
            f"- Attribution: {source['attribution']}.",
            f"- Obtained: {source['obtained']}. Original: [{source_path}]({source_path}).",
            f"- SHA-256: `{source['sha256']}`.", ""]
    lines += ["## One record per exported wave", ""]
    for cue in cues:
        path = Path(cue["path"]).relative_to(PRODUCTION.relative_to(ROOT)).as_posix()
        production = cue.get("production_source", f"game/audio/paint_exe/{path.split('/')[0]}/README.md")
        production = Path(production).relative_to(PRODUCTION.relative_to(ROOT)).as_posix()
        lines += [f"### {cue['id']}", "",
            f"- Bank name: `{cue['id']}`; cue: `{cue['cue_id']}`; Tier {cue['tier']}.",
            f"- Export: [{path}]({path}); loop: {'yes' if cue['loop'] else 'no'}.",
            f"- Creator/source: Codex arrangement/edit of {', '.join('`'+s+'`' for s in cue['source_ids'])}; exact creators and licences above.",
            "- Licence/permission: CC0 1.0 source recordings; MIT original arrangement/edit recipe.",
            "- Release ZIP and built-bank use: yes. Source WAV and editable-project redistribution: yes.",
            "- Attribution: none required for CC0 recordings; retain repository MIT notice for original code/arrangements.",
            f"- Modifications: {cue['description']} Exact trims, layer edits, gains and loop points in [{production}]({production}) and its renderer.",
            "- Export settings: 44.1 kHz, PCM16 master; see common settings above and machine-checked bank report.",
            f"- Production date: 2026-10-14. Status: {status}", ""]
    (PRODUCTION / "PROVENANCE.md").write_text("\n".join(lines), encoding="utf-8")


def read_pcm(path: Path) -> np.ndarray:
    with wave.open(str(path), "rb") as source:
        if (source.getframerate(), source.getsampwidth()) != (RATE, 2):
            raise ValueError(f"Unexpected format: {path}")
        pcm = np.frombuffer(source.readframes(source.getnframes()), dtype="<i2").astype(np.float64)
        pcm = pcm.reshape(-1, source.getnchannels()) / 32768
        return np.repeat(pcm, 2, axis=1) if pcm.shape[1] == 1 else pcm


def audition(cues: list[dict]) -> dict:
    by_id = {cue["id"]: cue for cue in cues}
    groups = {}
    for cue in cues:
        groups.setdefault(cue["cue_id"], []).append(cue)
    cache = {}
    directory = PRODUCTION / "auditions"
    directory.mkdir(exist_ok=True)
    metadata = {"kind": "offline scripted mix, not gameplay capture", "duration_seconds": 49,
                "fixed_output_gain": .65, "per_player_attenuation": False, "mixes": []}
    for players in (1, 2, 4):
        output = np.zeros((49 * RATE, 2))
        events = []
        def add(identifier, at, gain=1, duration=None, variant=0):
            cue = by_id.get(identifier) or groups[identifier][variant % len(groups[identifier])]
            if cue["id"] not in cache:
                cache[cue["id"]] = read_pcm(ROOT / cue["path"])
            signal = cache[cue["id"]]
            if duration is not None:
                frames = round(duration * RATE)
                if not cue["loop"]:
                    raise ValueError(f"Cannot extend nonlooping cue {identifier}")
                signal = np.tile(signal, (frames // len(signal) + 1, 1))[:frames].copy()
                fade = min(round(.02 * RATE), len(signal) // 2)
                signal[:fade] *= np.linspace(0, 1, fade)[:, None]
                signal[-fade:] *= np.linspace(1, 0, fade)[:, None]
            start = round(at * RATE)
            length = min(len(signal), len(output) - start)
            output[start:start + length] += signal[:length] * gain
            events.append({"cue": cue["id"], "at_seconds": at, "gain": gain,
                           "duration_seconds": duration or len(signal) / RATE})
        add("room.tone", 0, .5, 6)
        add("match.reveal_hover", .4)
        add("match.undo", 1)
        add("room.sigh", 1.8)
        add("room.chair", 3.1)
        add("room.footsteps", 4.1)
        add("music.close_quarters", 6, .65, 28)
        for at in (6, 7, 8):
            add("match.count_beat", at)
        add("match.glass_in", 8)
        add("match.go", 9)
        for player in range(players):
            offset = player * .073
            for step in range(16):
                at = 9.5 + step * .42 + offset
                add("player.step", at, .72, variant=step + player)
            for at in (13, 19, 25):
                add("player.jump", at + offset, .8)
                add("player.land", at + .7 + offset, .8, variant=player)
            weapon = ("sprayer", "roller", "mister", "sniper")[player]
            for shot, at in enumerate((11, 16.5, 22, 28)):
                at += offset
                if weapon == "sniper":
                    add("sniper.shot", at)
                    add("bucket.throw", at + 1.2)
                else:
                    add(weapon + ".start", at)
                    add(weapon + ".loop", at + .08, .72, 1.4)
                    add(weapon + ".stop", at + 1.48)
                for hit in range(3):
                    add("paint.splash", at + .2 + hit * .3, .65, variant=hit + shot + player)
            add("player.hurt", 23 + offset)
            add("weapon.empty", 26 + offset)
            add("weapon.recovered", 27.1 + offset)
        add("player.pop", 29.8)
        add("player.redraw", 30.4, .65, 1.5)
        add("player.respawn", 32)
        add("match.time_up", 34)
        add("match.glass_out", 34.1)
        add("room.tone", 36, .5, 8)
        add("room.chair", 37)
        add("match.reveal_hover", 38.8)
        add("match.reveal_hover", 40)
        add("match.reveal_click", 41.3)
        add("room.chair", 42.3, .65)
        add("match.celebrate", 44)
        add("match.slump", 44.25)
        add("results.winner", 46)
        output *= metadata["fixed_output_gain"]
        peak = float(np.max(np.abs(output)))
        if peak > 10 ** (-1 / 20):
            raise ValueError(f"{players}P audition clips/headroom fails: {peak}")
        encoded = np.rint(output * 32767).astype("<i2").tobytes()
        filename = f"mix-{players}p.ogg"
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-f", "s16le", "-ar", str(RATE),
                        "-ac", "2", "-i", "pipe:0", "-c:a", "libvorbis", "-q:a", "5",
                        "-map_metadata", "-1", str(directory / filename)], input=encoded, check=True)
        metadata["mixes"].append({"players": players, "path": "auditions/" + filename,
                                  "peak_dbfs": float(20 * np.log10(peak)), "events": events})
    save_json(directory / "timeline.json", metadata)
    return metadata


def review_page(cues: list[dict], mixes: dict) -> None:
    cards = []
    for cue in cues:
        path = Path(cue["path"]).relative_to(PRODUCTION.relative_to(ROOT)).as_posix()
        identifier = html.escape(cue["id"])
        cards.append(f'<article data-tier="{cue["tier"]}" data-search="{identifier}">'
            f'<div><h3>{identifier}</h3><span>Tier {cue["tier"]} · {"loop" if cue["loop"] else "one shot"}</span></div>'
            f'<p>{html.escape(cue["description"])}</p>'
            f'<audio controls preload="none" {"loop" if cue["loop"] else ""} src="{html.escape(path, quote=True)}"></audio>'
            f'<a href="{html.escape(path, quote=True)}" download>Download WAV</a></article>')
    mix_cards = ''.join(f'<article><h3>{mix["players"]} player{ "s" if mix["players"] > 1 else ""}</h3>'
        f'<audio controls preload="none" src="{mix["path"]}"></audio></article>' for mix in mixes["mixes"])
    page = '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ColourWars · Paint.exe audio review</title><style>
*{box-sizing:border-box}body{margin:0;background:#f3eee3;color:#25282d;font:16px/1.5 system-ui,sans-serif}
main{max-width:1160px;margin:auto;padding:35px 24px}h1{font-size:clamp(28px,5vw,48px);margin:0}h2{margin-top:35px}
h3{margin:0;font-size:16px;overflow-wrap:anywhere}p{max-width:85ch}a{color:#164b80}header{border-bottom:3px solid #25282d;padding-bottom:22px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(290px,1fr));gap:14px}article{padding:18px;background:#fffdf7;border:2px solid #25282d;box-shadow:3px 3px #d3cbb9}
article p{font-size:14px;min-height:42px}article span{font-size:13px;color:#62615d}article a{display:block;font-size:13px;margin-top:8px}audio{width:100%;margin-top:10px}
input,select{font:inherit;padding:10px;border:2px solid #25282d;background:white;max-width:100%}.filters{display:flex;gap:12px;flex-wrap:wrap;margin:20px 0;position:sticky;top:0;background:#f3eee3;padding:10px 0;z-index:1}
[hidden]{display:none!important}.status{background:#ffe6a5;border-left:5px solid #b06a00;padding:12px 16px}.small{font-size:14px}label{display:flex;gap:8px;align-items:center}
</style><main><header><p>COLOURWARS / POL#22 / 14 OCTOBER 2026</p><h1>Paint.exe audio review</h1>
<p>Recorded paint, paper and mechanisms. Original arrangements for ukulele, upright piano and hand percussion.</p>
<p class="status">ACCEPTANCE_STATUS In-game mix acceptance remains pending. These files do not change the game's live audio.</p>
<p><a href="README.md">Production notes</a> · <a href="PROVENANCE.md">Every source and licence</a> · <a href="auditions/timeline.json">Exact audition timeline</a></p></header>
<h2>Hear the set together</h2><p>Same 49-second scripted scene at 1, 2 and 4 players, using the same output gain. Intro room → countdown → movement and weapons → time-up → reveal. These are offline arrangements, not gameplay recordings. Useful cues should stay readable as players are added.</p>
<div class="grid">MIX_CARDS</div><p class="small">Check the sprayer, roller, mister, sniper and bucket by ear; compare movement and paint repetition; listen to the music joins. Tier 3 detail is optional and should default to silent or nearly silent. The Movement Course keeps no music or ambience.</p>
<h2>Individual cues</h2><div class="filters"><label>Find <input id="search" type="search" placeholder="e.g. sprayer, music, room"></label><label>Show <select id="tier"><option value="">All tiers</option><option value="1">Tier 1 first</option><option value="2">Tier 2</option><option value="3">Tier 3</option></select></label><span id="count" aria-live="polite"></span></div>
<div id="cues" class="grid">CUE_CARDS</div></main><script>
const search=document.querySelector('#search'),tier=document.querySelector('#tier'),cards=[...document.querySelectorAll('#cues article')];
function filter(){let shown=0;for(const card of cards){card.hidden=!(card.dataset.search.includes(search.value.toLowerCase())&&(!tier.value||card.dataset.tier===tier.value));if(!card.hidden)shown++;}document.querySelector('#count').textContent=shown+' cues';}
search.addEventListener('input',filter);tier.addEventListener('change',filter);filter();
document.addEventListener('play',event=>{if(event.target.tagName==='AUDIO')for(const audio of document.querySelectorAll('audio'))if(audio!==event.target)audio.pause();},true);
</script></html>'''.replace('MIX_CARDS', mix_cards).replace('CUE_CARDS', '\n'.join(cards)).replace('ACCEPTANCE_STATUS', html.escape(acceptance_status(cues)))
    (PRODUCTION / "index.html").write_text(page, encoding="utf-8")


def main() -> None:
    cues, sources = collect()
    provenance(cues, sources)
    review_page(cues, audition(cues))
    print(f"Prepared {len(cues)} cues, {len(sources)} source records, and 1/2/4-player offline mixes")


if __name__ == "__main__":
    main()
