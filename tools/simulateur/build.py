"""Génère testeur-led.html : aperçu simulé + pilotage du vrai bandeau.

Usage : python3 build.py <runs.json> <dossier des sons du sampler> <fichier de sortie>
Le dossier des sons est public/audio/sampler dans le dépôt de l'application.
"""
import base64
import json
import sys

runs_path, audio_dir, out_path = sys.argv[1:4]
runs = json.load(open(runs_path))['runs']


def rle(h):
    """Compresse une image du bandeau (6 caractères hexadécimaux par LED) en 'nombre*couleur|...'."""
    px = [h[i:i + 6] for i in range(0, len(h), 6)]
    out, prev, n = [], None, 0
    for p in px:
        if p == prev:
            n += 1
        else:
            if prev is not None:
                out.append(f'{n}*{prev}')
            prev, n = p, 1
    out.append(f'{n}*{prev}')
    return '|'.join(out)


for r in runs.values():
    for fr in r['f']:
        if len(fr[1]) > 6:
            fr[1] = rle(fr[1])

files = {'loup': 'loup', 'clocher': 'clocher', 'horloge': 'clock', 'tonnerre': 'tonnerre', 'violon': 'violon'}
audio = {k: 'data:audio/ogg;base64,' + base64.b64encode(open(f'{audio_dir}/sampler_{f}.ogg', 'rb').read()).decode()
         for k, f in files.items()}

html = open('template.html', encoding='utf-8').read()
html = html.replace('__RUNS__', json.dumps(runs, separators=(',', ':'))).replace('__AUDIO__', json.dumps(audio))
open(out_path, 'w', encoding='utf-8').write(html)
print(round(len(html) / 1e6, 2), 'Mo écrits dans', out_path)
