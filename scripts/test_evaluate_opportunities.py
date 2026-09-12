import unittest
from evaluate_opportunities import score

R, L = [[7, 1]], [[7, 0]]

def rows(change=None, flicker=False):
    return [dict(cycle=i, t=i/20, dt=.05, before=R,
                 **{'class': L if change is not None and i>=change and (not flicker or i<change+2) else R},
                 safety=False) for i in range(200)]

def oracle():
    return [dict(id='one', onset=0, confirmed=.5, deadline=3.5, expiry=10,
                 target=L, hold=.3, lock_window=5)]

class Tests(unittest.TestCase):
    def test_frozen(self):
        r=score(rows(),oracle(),10)
        self.assertEqual(r['warranted_switch_recall'],0)
        self.assertTrue(r['opportunities'][0]['persistent_lockup'])
    def test_timely(self):
        r=score(rows(20),oracle(),10)
        self.assertEqual(r['warranted_switch_recall'],1)
        self.assertAlmostEqual(r['opportunities'][0]['response_delay_s'],1)
    def test_late(self): self.assertEqual(score(rows(80),oracle(),10)['counts'],{'late':1})
    def test_flicker(self): self.assertEqual(score(rows(20,True),oracle(),10)['counts'],{'missed':1})
    def test_gap(self):
        r=rows();del r[10]
        self.assertIsNone(score(r,oracle(),10)['warranted_switch_recall'])
    def test_safety(self):
        r=rows();r[10]['safety']=True
        self.assertEqual(score(r,oracle(),10)['counts'],{'safety_interrupted':1})
    def test_competing(self):
        self.assertEqual(score(rows()[:20],oracle(),1,'collision')['warranted_switch_recall'],0)
    def test_safety_after_deadline(self):
        r=rows();r[100]['safety']=True
        self.assertEqual(score(r,oracle(),10)['warranted_switch_recall'],0)
    def test_expiry(self):
        o=oracle();o[0]['expiry']=2
        self.assertEqual(score(rows(),o,10)['counts'],{'opportunity_expired':1})
    def test_no_opportunities(self): self.assertIsNone(score(rows(),[],10)['warranted_switch_recall'])
    def test_partial(self): self.assertEqual(score(rows()[:20],oracle(),1)['counts'],{'insufficient_followup':1})
    def test_duplicate(self):
        with self.assertRaises(ValueError):score(rows(),oracle()*2,10)
    def test_initial_selection(self):
        self.assertEqual(score(rows(0),oracle(),10)['counts'],{'timely':1})
    def test_already_target(self):
        r=rows(0);r[0]['before']=L
        self.assertEqual(score(r,oracle(),10)['counts'],{'already_target':1})

if __name__=='__main__':unittest.main()
