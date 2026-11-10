"""Deterministic balance matrix jobs; called by run-balance-matrix.ps1 after one compile."""
import argparse
import concurrent.futures
import csv
import hashlib
import io
import json
import os
import re
import shlex
from pathlib import Path
import subprocess
import sys
import time
import threading
import uuid
from job_process import start_process

REPO = Path(__file__).resolve().parents[2]
TOOLS = ('sprayer', 'sniper', 'roller', 'mister', 'bucket')
SUBS = ('grenade', 'sprinkler', 'wall')
SECTIONS = ('weapons.', 'projectiles.', 'subs.', 'bots.')

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest().upper()

def write_json(path, value):
    temp = path.with_name(path.name + '.' + uuid.uuid4().hex + '.tmp')
    with temp.open('x',encoding='utf-8',newline='\n') as stream:
        stream.write(json.dumps(value, indent=2, sort_keys=True) + '\n')
    os.replace(temp, path)

def safe_name(value):
    return isinstance(value, str) and value and all(c.isascii() and (c.isalnum() or c in '-_') for c in value)

def design_jobs(design):
    known = {'Schema', 'Name', 'Stages', 'SeedStart', 'SeedCount', 'Duration', 'Difficulties',
             'Tools', 'Rosters', 'Variants', 'ComparisonMargins'}
    if set(design) - known or design.get('Schema') != 1 or not safe_name(design.get('Name')):
        raise ValueError('Design needs Schema=1, an identifier Name and only documented keys')
    stages, difficulties = design['Stages'], design['Difficulties']
    tools = design.get('Tools', list(TOOLS))
    if not stages or len(stages) > 16 or len(set(stages)) != len(stages) or not all(safe_name(x) for x in stages):
        raise ValueError('Stages must name 1..16 unique identifiers')
    if not difficulties or len(set(difficulties)) != len(difficulties) or set(difficulties) - {'easy','normal','hard'}:
        raise ValueError('Difficulties must be unique easy/normal/hard levels')
    if not tools or len(set(tools)) != len(tools) or set(tools) - set(TOOLS):
        raise ValueError('Tools must be unique concrete weapons')
    start, count = design['SeedStart'], design['SeedCount']
    if type(start) is not int or type(count) is not int or not 1 <= count <= 64 or not 0 <= start <= 2**32-count:
        raise ValueError('A consecutive seed block needs 1..64 seeds within uint32')
    duration = design['Duration']
    if type(duration) not in (int,float) or not .05 <= duration <= 600:
        raise ValueError('Duration must be finite and 0.05..600 seconds')
    margins = design.get('ComparisonMargins', {})
    if set(margins) - {'paint','kills','deaths'} or any(type(x) not in (float,int) or not 0 <= x < float('inf') for x in margins.values()):
        raise ValueError('ComparisonMargins must contain finite nonnegative paint/kills/deaths margins')
    cases = []
    for roster in design['Rosters']:
        if set(roster) - {'Kind','Painters','Tools','Subs','Name'}:
            raise ValueError('Unknown roster key')
        kind = roster['Kind']
        if kind == 'ordered_pairs':
            for a in tools:
                for b in tools:
                    cases.append((f'{a}-{b}', [a,b], roster.get('Subs',['grenade','grenade'])))
        elif kind == 'tool_with_painter':
            painters = roster['Painters']
            if not painters or len(set(painters)) != len(painters) or set(painters)-set(TOOLS):
                raise ValueError('Painters must name unique concrete tools')
            for tool in tools:
                for painter in painters:
                    # Team A: tool + painter; team B: painter + painter.
                    a, b = [tool,painter,painter,painter], [painter,tool,painter,painter]
                    cases.append((f'{tool}-with-{painter}-a', a, roster.get('Subs',['grenade']*4)))
                    if a != b:
                        cases.append((f'{tool}-with-{painter}-b', b, roster.get('Subs',['grenade']*4)))
        elif kind == 'explicit':
            cases.append((roster['Name'],roster['Tools'],roster.get('Subs',['grenade']*len(roster['Tools']))))
        else:
            raise ValueError('Roster Kind must be ordered_pairs, tool_with_painter or explicit')
    if not cases or len({(tuple(t), tuple(s)) for _,t,s in cases}) != len(cases):
        raise ValueError('Rosters must produce unique nonempty tool/sub loadouts')
    if any(not safe_name(n) or not 1<=len(t)<=4 or set(t)-set(TOOLS) or len(t)!=len(s) or set(s)-set(SUBS) for n,t,s in cases):
        raise ValueError('Each loadout needs an identifier, 1..4 tools, and one concrete sub per tool')
    variants = design.get('Variants',[{'Name':'baseline','Patch':{}}])
    if not variants or len({v['Name'] for v in variants}) != len(variants):
        raise ValueError('Variants need unique names')
    jobs=[]
    for variant in variants:
        if set(variant) != {'Name','Patch'} or not safe_name(variant['Name']) or not isinstance(variant['Patch'],dict):
            raise ValueError('Each variant needs Name and a flat Patch object')
        for key,value in variant['Patch'].items():
            if not any(key.startswith(p) for p in SECTIONS) or type(value) not in (int,float,bool) or (type(value) is float and not abs(value)<float('inf')):
                raise ValueError('Patches contain only finite numeric/boolean dotted keys under weapons/projectiles/subs/bots')
        for stage in stages:
            for difficulty in difficulties:
                for name, loadout, subs in cases:
                    jobs.append({'id':f"{variant['Name']}-{stage}-{difficulty}-{name}", 'variant':variant['Name'],
                                 'stage':stage,'difficulty':difficulty,'tools':loadout,'subs':subs,
                                 'seeds':list(range(start,start+count)), 'duration':duration,
                                 'patch_sha256':hashlib.sha256((json.dumps(variant['Patch'],indent=2,sort_keys=True)+'\n').encode()).hexdigest().upper()})
    if len({j['id'] for j in jobs}) != len(jobs) or len(jobs)>10000:
        raise ValueError('Design must have unique job ids and no more than 10000 jobs')
    return jobs, variants

def output_path(path):
    out = Path(path).resolve()
    try:
        out.relative_to((REPO/'out').resolve())
    except ValueError:
        raise ValueError('Matrix evidence must be under this checkout\'s out/ directory') from None
    if out == (REPO/'out').resolve():
        raise ValueError('Choose a dedicated matrix output directory under out/')
    return out

def runtime_pins(content, policy):
    # This is the same reviewed allowlist staging/package consume. Source
    # content directories may also contain private legacy audio or source art.
    text=policy.read_text(encoding='utf-8-sig')
    def entries(name):
        found=re.search(r"set\(\s*"+name+r"\s+([^)]*)\)",text,re.MULTILINE)
        if found is None:
            raise ValueError('Runtime policy has no '+name)
        return shlex.split(found.group(1),comments=True,posix=True)
    assets=entries('COLOURWARS_RUNTIME_ASSETS')
    optional=set(entries('COLOURWARS_OPTIONAL_RUNTIME_ASSETS'))
    if not assets or len(set(assets))!=len(assets) or not optional.issubset(assets):
        raise ValueError('Runtime policy assets must be unique and optional assets must be listed')
    pins={}
    for relative in assets:
        name=Path(relative)
        if name.is_absolute() or '..' in name.parts or not relative or '\\' in relative:
            raise ValueError('Runtime policy needs plain relative asset paths')
        path=content/name
        pins[str(path.resolve())]=None if relative in optional and not path.exists() else digest(path)
    return pins

def verify_pins(pins):
    for path,expected in pins.items():
        if (Path(path).exists() if expected is None else digest(path)!=expected):
            raise ValueError('Input changed during the matrix: '+path)

def pin_inputs(exe, content, design_source):
    paths = [exe, content/'tuning.json', content/'presentation.json', content/'manifest.json',
             content/'ColourWars.exe', REPO/'out/build/x64-debug/generated/build-info.txt',
             Path(__file__), Path(__file__).with_name('job_process.py'), REPO/'scripts/run-balance-matrix.ps1', design_source]
    policy=REPO/'cmake/runtime_assets.cmake'
    paths.append(policy)
    paths += sorted((REPO/'scripts/acceptance/balance_runner').glob('*.*'))
    paths += [REPO/'scripts/acceptance/balance_runner.cpp', REPO/'out/build/x64-debug/game/colourwars_game.lib']
    paths += [REPO/'scripts/acceptance/balance_regions'/n for n in ('metrics.h','metrics.cpp','csv.cpp')]
    paths += sorted(p for p in (REPO/'game').rglob('*') if p.is_file() and p.suffix in ('.cpp','.h'))
    pins={str(p.resolve()):digest(p) for p in paths}
    pins.update(runtime_pins(content,policy))
    return pins

def valid_result(info, directory, signature):
    try:
        return info['outcome']=='passed' and info['signature']==signature and all(
            digest(directory/name)==value for name,value in info['outputs'].items())
    except (KeyError,OSError):
        return False

def owned_path(out, relative):
    path=out/relative
    if os.path.normcase(str(path.resolve())) != os.path.normcase(os.path.abspath(path)):
        raise ValueError('Matrix refuses linked evidence paths: '+str(path))
    return path

class RunControl:
    def __init__(self):
        self.cancelled=threading.Event()
        self.lock=threading.Lock()
        self.active=set()
    def cancel(self):
        self.cancelled.set()
        with self.lock:
            for process in self.active:
                if process.poll() is None:
                    process.terminate()

def process_alive(pid):
    if os.name=='nt':
        import ctypes
        kernel=ctypes.WinDLL('kernel32',use_last_error=True)
        kernel.OpenProcess.restype=ctypes.c_void_p
        kernel.CloseHandle.argtypes=[ctypes.c_void_p]
        handle=kernel.OpenProcess(0x1000,False,pid)
        if not handle:
            return ctypes.get_last_error()!=87 # Invalid PID is gone; access denied stays owned.
        kernel.CloseHandle(handle)
        return True
    try:
        os.kill(pid,0)
        return True
    except ProcessLookupError:
        return False

def acquire_lock(path):
    for _ in range(3):
        try:
            fd=os.open(path,os.O_WRONLY|os.O_CREAT|os.O_EXCL)
            os.write(fd,json.dumps({'pid':os.getpid()}).encode());os.close(fd)
            return
        except FileExistsError:
            try:
                owner=json.loads(path.read_text())
                pid=owner['pid'] if isinstance(owner,dict) else owner
                if type(pid) is not int or pid<=0 or process_alive(pid):
                    raise ValueError('Matrix output is locked by a live or unverified owner: '+str(path))
                path.rename(path.with_name('matrix.lock.stale-'+str(time.time_ns())))
            except FileNotFoundError:
                continue
    raise ValueError('Could not acquire matrix output lock: '+str(path))

def prepare_job_roots(out, jobs):
    # Resolve/create shared parents before any worker can create them. Windows
    # non-strict path resolution races when a missing parent appears midway
    # through resolving another worker's absent child path.
    for relative in ('jobs','patches','variants'):
        path=owned_path(out,relative)
        path.mkdir(exist_ok=True)
        owned_path(out,relative)
    # Preflight every job destination serially as well, so a linked destination
    # is refused before simulation and worker checks see stable existing paths.
    for job in jobs:
        path=owned_path(out,Path('jobs')/job['id'])
        path.mkdir(exist_ok=True)
        owned_path(out,Path('jobs')/job['id'])

def run_job(job, out, exe, content, signature, timeout, control=None):
    if control is not None and control.cancelled.is_set():
        raise RuntimeError('Matrix cancelled before queued job started')
    base=owned_path(out,Path('jobs')/job['id'])
    base.mkdir(parents=True,exist_ok=True)
    receipt=base/'job.json'
    if receipt.exists():
        prior=json.loads(receipt.read_text(encoding='utf-8'))
        if valid_result(prior,base/prior['attempt'],signature):
            return job, base/prior['attempt'], True
    attempt=base/f'attempt-{time.time_ns()}'
    attempt.mkdir()
    patch_path=owned_path(out,Path('patches')/(job['variant']+'.json'))
    if digest(patch_path)!=job['patch_sha256']:
        raise ValueError('Variant patch changed before job: '+str(patch_path))
    args=[str(exe),str(content),'--stage',job['stage'],'--players',str(len(job['tools'])),
          '--duration',str(job['duration']),'--tools',','.join(job['tools']),'--subs',','.join(job['subs']),
          '--bot','match','--difficulty',','.join([job['difficulty']]*len(job['tools'])),
          '--tuning-patch',str(patch_path),
          '--csv',str(attempt/'results.csv'),'--paint-csv',str(attempt/'results.paint.csv')]
    for seed in job['seeds']:
        args+=['--seed',str(seed)]
    info={'signature':signature,'job':job,'arguments':args,'attempt':attempt.name,'outcome':'running',
          'started_utc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())}
    write_json(receipt,info)
    start=time.monotonic()
    env=os.environ.copy();env['PATH']=str(content)+os.pathsep+env['PATH'];env['COLOURWARS_MUTE']='1'
    with (attempt/'run.log').open('wb') as log:
        try:
            # The native runner has no child processes. On a deadline this owns
            # and terminates only that directly-created process.
            process=start_process(args,log,env)
            if control is not None:
                with control.lock:
                    control.active.add(process)
                    if control.cancelled.is_set():
                        process.terminate()
            try:
                try:
                    code=process.wait(timeout=timeout)
                except BaseException:
                    if process.poll() is None:
                        process.terminate()
                    process.wait()
                    raise
            finally:
                if control is not None:
                    with control.lock:
                        control.active.discard(process)
            info['exit_code']=code
            if code:
                raise RuntimeError(f"{job['id']} exited {code}; inspect {attempt/'run.log'}")
            if digest(patch_path)!=job['patch_sha256']:
                raise ValueError('Variant patch changed during job: '+str(patch_path))
            info['outputs']={name:digest(attempt/name) for name in ('results.csv','results.paint.csv')}
            info['outcome']='passed'
        except BaseException as error:
            info['outcome']='failed';info['failure']=str(error)
            raise
        finally:
            info['seconds']=time.monotonic()-start
            write_json(receipt,info)
    return job,attempt,False

def aggregate(out, completed, variant):
    main,paint=[],[]
    offset=0
    for job,directory,_ in sorted(completed,key=lambda x:x[0]['id']):
        if job['variant'] != variant:
            continue
        with (directory/'results.csv').open(encoding='utf-8-sig',newline='') as f:
            rows=list(csv.reader(f))
        with (directory/'results.paint.csv').open(encoding='utf-8-sig',newline='') as f:
            regions=list(csv.reader(f))
        if not main:
            main.append(rows[0]);paint.append(regions[0])
        if main[0]!=rows[0] or paint[0]!=regions[0]:
            raise ValueError('Job CSV schemas differ')
        main.extend(rows[1:])
        index=regions[0].index('match_row')
        for row in regions[1:]:
            row[index]=str(int(row[index])+offset)
            paint.append(row)
        offset+=len(rows)-1
    target=owned_path(out,Path('variants')/variant)
    target.mkdir(parents=True,exist_ok=True)
    for name,rows in [('matches.paint.csv',paint),('matches.csv',main)]:
        temp=target/(name+'.'+uuid.uuid4().hex+'.tmp')
        with temp.open('x',encoding='utf-8',newline='') as f:
            csv.writer(f,lineterminator='\n').writerows(rows)
        os.replace(temp,target/name)
    return offset

def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--design',type=Path)
    parser.add_argument('--design-stdin',action='store_true')
    parser.add_argument('--design-source',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--exe',type=Path)
    parser.add_argument('--content',type=Path,default=REPO/'out/build/x64-debug/game')
    parser.add_argument('--parallel',type=int,default=4)
    parser.add_argument('--timeout',type=int,default=3600)
    parser.add_argument('--plan-only',action='store_true')
    args=parser.parse_args(argv)
    try:
        design=json.load(sys.stdin) if args.design_stdin else json.loads(args.design.read_text(encoding='utf-8-sig'))
        jobs,variants=design_jobs(design);out=output_path(args.out)
        if not 1<=args.parallel<=16 or not 1<=args.timeout<=86400:
            raise ValueError('Parallel needs 1..16; timeout needs 1..86400 seconds')
        print(f"{design['Name']}: {len(jobs)} jobs, {sum(len(j['seeds']) for j in jobs)} matches, {args.parallel} workers",flush=True)
        if args.plan_only:
            return 0
        if args.exe is None or not args.exe.is_file():
            raise ValueError('A compiled Debug runner --exe is required')
        pins=pin_inputs(args.exe,args.content,args.design_source)
        signature=hashlib.sha256(json.dumps({'design':design,'pins':pins},sort_keys=True).encode()).hexdigest()
        out.mkdir(parents=True,exist_ok=True)
        lock=out/'matrix.lock'
        acquire_lock(lock)
        info=None
        try:
            receipt=out/'matrix-info.json'
            if receipt.exists() and json.loads(receipt.read_text())['signature']!=signature:
                raise ValueError('Resume refused: design, executable, content or source pins changed; choose a new output directory')
            info={'schema_version':1,'signature':signature,'design':design,'input_pins':pins,'outcome':'running',
                  'jobs':len(jobs),'matches':sum(len(j['seeds']) for j in jobs),'device_free':True,'human_acceptance':False}
            write_json(receipt,info)
            prepare_job_roots(out,jobs+[dict(id='identity-repeat')])
            for variant in variants:
                write_json(out/'patches'/(variant['Name']+'.json'),variant['Patch'])
            completed=[]
            control=RunControl()
            pool=concurrent.futures.ThreadPoolExecutor(max_workers=args.parallel)
            try:
                futures=[pool.submit(run_job,j,out,args.exe,args.content,signature,args.timeout,control) for j in jobs]
                for future in concurrent.futures.as_completed(futures):
                    done=future.result();completed.append(done)
                    print(f"[{len(completed)}/{len(jobs)}] {done[0]['id']} {'resumed' if done[2] else 'passed'}",flush=True)
            except BaseException:
                control.cancel()
                for future in futures:
                    future.cancel()
                raise
            finally:
                pool.shutdown(wait=True,cancel_futures=True)
            # Exact rerun is outside the aggregates and names no matches.csv.
            check=dict(jobs[0]);check['id']='identity-repeat';check['seeds']=jobs[0]['seeds']
            repeated=run_job(check,out,args.exe,args.content,signature,args.timeout)
            original=next(x for x in completed if x[0]['id']==jobs[0]['id'])
            for name in ('results.csv','results.paint.csv'):
                if digest(repeated[1]/name)!=digest(original[1]/name):
                    raise ValueError('Identity repeat changed '+name)
            for variant in variants:
                expected=next(j['patch_sha256'] for j in jobs if j['variant']==variant['Name'])
                if digest(out/'patches'/(variant['Name']+'.json'))!=expected:
                    raise ValueError('Variant patch changed during matrix: '+variant['Name'])
            verify_pins(pins)
            info['variant_matches']={v['Name']:aggregate(out,completed,v['Name']) for v in variants}
            info['identity_repeat']='byte-identical main and paint CSVs'
            info['outcome']='passed';write_json(receipt,info)
            print(f'PASS matrix; aggregates: {out / "variants"}',flush=True)
        except BaseException as error:
            if info is not None:
                info['outcome']='failed';info['failure']=str(error);write_json(receipt,info)
            raise
        finally:
            lock.unlink()
        return 0
    except (ValueError,OSError,RuntimeError,subprocess.TimeoutExpired) as error:
        print('matrix.py: '+str(error),file=sys.stderr)
        return 1

if __name__=='__main__':
    raise SystemExit(main())
