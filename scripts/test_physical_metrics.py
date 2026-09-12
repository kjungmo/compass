import unittest
from physical_metrics import score_physical

def rows(n=30):
    return [dict(cycle=i,t=i*.1,dt=.1,speed_mps=0.,goal_distance_m=5.,min_clearance_m=1.,
                 free_motion_warranted=True,planned_stop=False,collision=False,goal_reached=False) for i in range(n)]
class PhysicalTests(unittest.TestCase):
    def test_freeze(self):
        s=score_physical(rows(),'timeout',3);self.assertEqual(s['freezing_episodes'],1);self.assertAlmostEqual(s['freezing_duration_s'],3)
    def test_planned_stop(self):
        r=rows()
        for x in r:x['planned_stop']=True
        self.assertEqual(score_physical(r,'timeout',3)['freezing_episodes'],0)
    def test_no_warrant(self):
        r=rows()
        for x in r:x['free_motion_warranted']=False
        self.assertEqual(score_physical(r,'timeout',3)['freezing_episodes'],0)
    def test_short(self):self.assertEqual(score_physical(rows(19),'timeout',1.9)['freezing_episodes'],0)
    def test_missing(self):
        r=rows();del r[5]
        with self.assertRaises(ValueError):score_physical(r,'timeout',3)
    def test_truncated(self):
        with self.assertRaises(ValueError):score_physical(rows(10),'timeout',3)
    def test_goal(self):
        r=rows();r[-1].update(goal_reached=True,goal_distance_m=.1)
        self.assertTrue(score_physical(r,'goal',60)['success'])
    def test_collision_not_success(self):
        r=rows();r[-1].update(collision=True,goal_reached=True,goal_distance_m=.1)
        with self.assertRaises(ValueError):score_physical(r,'goal',60)
        self.assertFalse(score_physical(r,'collision',60)['success'])
if __name__=='__main__':unittest.main()
