import importlib.util
from pathlib import Path
import unittest
spec = importlib.util.spec_from_file_location('aliases', Path(__file__).resolve().parents[1] / 'tools/generate_artwork_aliases.py')
aliases = importlib.util.module_from_spec(spec)
spec.loader.exec_module(aliases)

class ArtworkAliases(unittest.TestCase):
    def test_case_punctuation_and_generic_release(self):
        rows = aliases.resolve({'kof98': "The King of Fighters '98 / Dream Match (NGM-2420)", 'dino': 'Dino: Adventure (World)'}, ["Named_Boxarts/The King of Fighters '98 _ dream match (NGM-2420).png", 'Named_Boxarts/Dino_ Adventure.png'])
        self.assertIn(('kof98', 'Named_Boxarts', "The King of Fighters '98 _ dream match (NGM-2420)"), rows)
        self.assertIn(('dino', 'Named_Boxarts', 'Dino_ Adventure'), rows)
    def test_exact_names_need_no_alias(self):
        self.assertEqual(aliases.resolve({'dino': 'Dino: Adventure'}, ['Named_Boxarts/Dino_ Adventure.png']), [])
    def test_ambiguous_and_different_sequels_do_not_match(self):
        self.assertEqual(aliases.resolve({'game': 'Game: Two', 'kof98': 'King of Fighters 98'}, ['Named_Boxarts/Game- Two.png', 'Named_Boxarts/Game_ Two.png', 'Named_Boxarts/King of Fighters 99.png']), [])
    def test_no_cross_category_or_variant_guess(self):
        self.assertEqual(aliases.resolve({'dino': 'Dino (World)'}, ['Named_Logos/Dino.png', 'Named_Boxarts/Dino (USA).png']), [])

if __name__ == '__main__':
    unittest.main()
