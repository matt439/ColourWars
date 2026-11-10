import csv
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import matrix


def design():
    return dict(Schema=1,Name='test',Stages=['arena'],SeedStart=7,SeedCount=3,Duration=1,
                Difficulties=['normal'],Tools=['sprayer','sniper'],
                Rosters=[dict(Kind='ordered_pairs')],Variants=[dict(Name='baseline',Patch={})],
                ComparisonMargins=dict(paint=.03,kills=.2,deaths=.2))

class FakeProcess:
    calls=0
    def __init__(self,args,**kwargs):
        type(self).calls+=1
        main=Path(args[args.index('--csv')+1]);paint=Path(args[args.index('--paint-csv')+1])
        main.write_text('stage,seed\narena,7\n',encoding='utf-8')
        paint.write_text('match_row,area\n1,4\n',encoding='utf-8')
    def wait(self,timeout=None): return 0
    def poll(self): return 0
    def terminate(self): raise AssertionError('completed fake must not be killed')

class MatrixTests(unittest.TestCase):
    def test_cartesian_consecutive_seeds_and_side_swaps(self):
        jobs,_=matrix.design_jobs(design())
        self.assertEqual(len(jobs),4)
        self.assertTrue(all(j['seeds']==[7,8,9] for j in jobs))
        self.assertEqual({tuple(j['tools']) for j in jobs},{('sprayer','sprayer'),('sprayer','sniper'),('sniper','sprayer'),('sniper','sniper')})
        d=design();d['Rosters']=[dict(Kind='tool_with_painter',Painters=['roller'])]
        jobs,_=matrix.design_jobs(d)
        self.assertEqual(len(jobs),4)
        for j in jobs:
            self.assertEqual(j['tools'].count('roller'),3)
            self.assertEqual(len(j['subs']),4)
    def test_rejects_bad_design_before_jobs(self):
        for key,value in [('SeedStart',2**32),('SeedCount',0),('Duration',float('nan')),('Stages',['../escape']),('Unknown',True)]:
            d=design();d[key]=value
            with self.subTest(key=key),self.assertRaises(ValueError): matrix.design_jobs(d)
        for patch_value in ({'player.gravity':1},{'bots.hard.reaction_steps':'fast'},{'subs.grenade.cost':float('inf')}):
            d=design();d['Variants'][0]['Patch']=patch_value
            with self.assertRaises(ValueError): matrix.design_jobs(d)
    def test_resumes_only_hashed_success_and_preserves_failed_bytes(self):
        with tempfile.TemporaryDirectory() as temp,patch.object(matrix,'start_process',side_effect=lambda args,log,env: FakeProcess(args)):
            FakeProcess.calls=0
            out=Path(temp);(out/'patches').mkdir();matrix.write_json(out/'patches/baseline.json',{})
            job=matrix.design_jobs(design())[0][0]
            first=matrix.run_job(job,out,Path('runner'),Path('content'),'signature',10)
            self.assertFalse(first[2]);self.assertEqual(FakeProcess.calls,1)
            again=matrix.run_job(job,out,Path('runner'),Path('content'),'signature',10)
            self.assertTrue(again[2]);self.assertEqual(FakeProcess.calls,1)
            (first[1]/'results.csv').write_text('broken')
            replaced=matrix.run_job(job,out,Path('runner'),Path('content'),'signature',10)
            self.assertEqual(FakeProcess.calls,2);self.assertNotEqual(first[1],replaced[1])
            self.assertEqual((first[1]/'results.csv').read_text(),'broken')
    def test_companion_join_offsets_follow_aggregate_main_rows(self):
        with tempfile.TemporaryDirectory() as temp,patch.object(matrix,'start_process',side_effect=lambda args,log,env: FakeProcess(args)):
            out=Path(temp);(out/'patches').mkdir();matrix.write_json(out/'patches/baseline.json',{})
            jobs=matrix.design_jobs(design())[0][:2]
            completed=[matrix.run_job(j,out,Path('runner'),Path('content'),'sig',10) for j in jobs]
            self.assertEqual(matrix.aggregate(out,completed,'baseline'),2)
            with (out/'variants/baseline/matches.paint.csv').open(newline='') as f:
                self.assertEqual([r['match_row'] for r in csv.DictReader(f)],['1','2'])
    def test_stale_lock_resumes_but_live_lock_refuses(self):
        with tempfile.TemporaryDirectory() as temp:
            lock=Path(temp)/'matrix.lock';lock.write_text(json.dumps({'pid':999999}))
            with patch.object(matrix,'process_alive',return_value=False): matrix.acquire_lock(lock)
            self.assertEqual(json.loads(lock.read_text())['pid'],__import__('os').getpid())
            self.assertEqual(len(list(lock.parent.glob('matrix.lock.stale-*'))),1)
            with patch.object(matrix,'process_alive',return_value=True),self.assertRaises(ValueError): matrix.acquire_lock(lock)
    def test_shared_and_job_roots_are_stable_before_parallel_resolution(self):
        import concurrent.futures
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);jobs=[dict(id='job-'+str(i)) for i in range(64)]
            matrix.prepare_job_roots(root,jobs)
            self.assertTrue(all((root/name).is_dir() for name in ('jobs','patches','variants')))
            def resolve(job):
                path=matrix.owned_path(root,Path('jobs')/job['id'])
                (path/'attempt').mkdir()
                return path
            with concurrent.futures.ThreadPoolExecutor(max_workers=16) as pool:
                found=list(pool.map(resolve,jobs))
            self.assertEqual(len(set(found)),64)
            self.assertTrue(all(path.is_dir() for path in found))

    def test_output_must_stay_under_checkout_out(self):
        with self.assertRaises(ValueError): matrix.output_path(matrix.REPO/'docs/escape')
        with self.assertRaises(ValueError): matrix.output_path(matrix.REPO/'out')
        self.assertEqual(matrix.output_path(matrix.REPO/'out/balance/test'),(matrix.REPO/'out/balance/test').resolve())

class RuntimePinsTests(unittest.TestCase):
    def test_only_reviewed_assets_and_absent_optional_state_are_pinned(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);content=root/'content';content.mkdir()
            policy=root/'runtime_assets.cmake'
            policy.write_text('set(COLOURWARS_RUNTIME_ASSETS tuning.json sounds/bank.xwb)\nset(COLOURWARS_OPTIONAL_RUNTIME_ASSETS sounds/bank.xwb)\n')
            (content/'tuning.json').write_text('{}')
            (content/'unreviewed-private.wav').write_bytes(b'not staged content')
            pins=matrix.runtime_pins(content,policy)
            self.assertEqual(set(pins),{str((content/'tuning.json').resolve()),str((content/'sounds/bank.xwb').resolve())})
            self.assertIsNone(pins[str((content/'sounds/bank.xwb').resolve())])
            matrix.verify_pins(pins)
            (content/'sounds').mkdir();(content/'sounds/bank.xwb').write_bytes(b'added optional')
            with self.assertRaisesRegex(ValueError,'Input changed'): matrix.verify_pins(pins)
    def test_missing_required_reviewed_asset_fails(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);policy=root/'runtime_assets.cmake'
            policy.write_text('set(COLOURWARS_RUNTIME_ASSETS required.json sounds/bank.xwb)\nset(COLOURWARS_OPTIONAL_RUNTIME_ASSETS sounds/bank.xwb)\n')
            with self.assertRaises(FileNotFoundError): matrix.runtime_pins(root,policy)

if __name__=='__main__': unittest.main()
