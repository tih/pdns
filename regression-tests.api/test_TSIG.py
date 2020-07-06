from __future__ import print_function
import json
import time
import unittest
from copy import deepcopy
from pprint import pprint
from test_helper import ApiTestCase, unique_tsigkey_name, is_auth, is_recursor, get_db_tsigkeys

class AuthTSIGHelperMixin(object):
    def create_tsig_key(self, name=None, algorithm='hmac-md5', key=None):
        if name is None:
            name = unique_tsigkey_name()
        payload = {
            'name': name,
            'algorithm': algorithm,
        }
        if key is not None:
            payload.update({'key': key})
        print("sending", payload)
        r = self.session.post(
            self.url("/api/v1/servers/localhost/tsigkeys"),
            data=json.dumps(payload),
            headers={'content-type': 'application/json'})
        self.assert_success_json(r)
        self.assertEquals(r.status_code, 201)
        reply = r.json()
        print("reply", reply)
        return name, payload, reply


@unittest.skipIf(not is_auth(), "Not applicable")
class AuthTSIG(ApiTestCase, AuthTSIGHelperMixin):
    def test_create_key(self):
        """
        Create a TSIG key that is generated by the server
        """
        name, payload, data = self.create_tsig_key()
        for k in ('id', 'name', 'algorithm', 'key', 'type'):
            self.assertIn(k, data)
            if k in payload:
                self.assertEquals(data[k], payload[k])

    def test_create_key_with_key_data(self):
        """
        Create a new key with the key data provided
        """
        key = 'fn+BREHMDq0uWA1WbDwaoc2ne3rD973ySJ33ToJTfWY='
        name, payload, data = self.create_tsig_key(key=key)
        self.assertEqual(data['key'], key)

    def test_create_key_with_hmacsha512(self):
        """
        Have the server generate a key with the provided algorithm
        """
        algorithm = 'hmac-sha512'
        name, payload, data = self.create_tsig_key(algorithm=algorithm)
        self.assertEqual(data['algorithm'], algorithm)

    def test_get_non_existing_key(self):
        """
        Try to get get a key that does not exist
        """
        name = "idonotexist"
        r = self.session.get(self.url(
            "/api/v1/servers/localhost/tsigkeys/" + name + '.'),
            headers={'accept': 'application/json'})
        self.assert_error_json(r)
        self.assertEqual(r.status_code, 404)
        newdata = r.json()
        self.assertIn('TSIG key with name \'' + name + '\' not found', newdata['error'])

    def test_remove_key(self):
        """
        Create a key and attempt to delete it
        """
        name, payload, data = self.create_tsig_key()
        r = self.session.delete(self.url("/api/v1/servers/localhost/tsigkeys/" + data['id']))
        self.assertEqual(r.status_code, 204)
        keys_from_db = get_db_tsigkeys(name)
        self.assertListEqual(keys_from_db, [])

    def test_put_key_change_name(self):
        """
        Rename a key by PUTing a json with "name" set
        """
        name, payload, data = self.create_tsig_key()
        payload = {
            'name': 'mynewkey'
        }
        r = self.session.put(self.url("/api/v1/servers/localhost/tsigkeys/" + data['id']),
                             data=json.dumps(payload))
        self.assertEqual(r.status_code, 200)
        newdata = r.json()
        self.assertEqual(newdata['name'], 'mynewkey')

        # Check if the old key is removed
        r = self.session.get(self.url("/api/v1/servers/localhost/tsigkeys/" + data['id']))
        self.assertEqual(r.status_code, 404, "Old key was not removed!")

    def test_put_key_change_key(self):
        """
        Change the key by PUTing it
        """
        name, payload, data = self.create_tsig_key()
        newkey = 'l36TAJalAys0HeEfSM1rFzSmz9kSwfiBo3HNkL62COs='
        payload = {
            'key': newkey
        }
        r = self.session.put(self.url("/api/v1/servers/localhost/tsigkeys/" + data['id']),
                             data=json.dumps(payload))
        self.assertEqual(r.status_code, 200)
        data = r.json()
        self.assertEqual(data['key'], newkey)

    def test_put_key_change_algo(self):
        name, payload, data = self.create_tsig_key()
        newalgo = 'hmac-sha256'
        payload = {
            'algorithm': newalgo
        }
        r = self.session.put(self.url("/api/v1/servers/localhost/tsigkeys/" + data['id']),
                             data=json.dumps(payload))
        self.assertEqual(r.status_code, 200)
        data = r.json()
        self.assertEqual(data['algorithm'], newalgo)

    def test_put_non_existing_algo(self):
        name, payload, data = self.create_tsig_key()
        payload = {
            'algorithm': 'foobar'
        }
        r = self.session.put(self.url("/api/v1/servers/localhost/tsigkeys/" + data['id']),
                             data=json.dumps(payload))
        self.assertEqual(r.status_code, 422)
        data = r.json()
        self.assertIn('Unknown TSIG algorithm: ', data['error'])

    def test_put_broken_key(self):
        name, payload, data = self.create_tsig_key()
        payload = {
            'key': 'f\u0333oobar1======'
        }
        r = self.session.put(self.url("/api/v1/servers/localhost/tsigkeys/" + data['id']),
                             data=json.dumps(payload))
        data = r.json()
        self.assertEqual(r.status_code, 422)
        self.assertIn('Can not base64 decode key content ', data['error'])

    def test_put_to_non_existing_key(self):
        name = unique_tsigkey_name()
        payload = {
            'algorithm': 'hmac-sha512'
        }
        r = self.session.put(self.url("/api/v1/servers/localhost/tsigkeys/" + name + '.'),
                             data=json.dumps(payload),
                             headers={'accept': 'application/json'})
        self.assertEqual(r.status_code, 404)
        data = r.json()
        self.assertIn('TSIG key with name \'' + name + '\' not found', data['error'])

    def test_post_existing_key_name(self):
        name, payload, data = self.create_tsig_key()
        r = self.session.post(self.url("/api/v1/servers/localhost/tsigkeys"),
                              headers={'accept': 'application/json'},
                              data=json.dumps(payload))
        self.assertEqual(r.status_code, 409)
        data = r.json()
        self.assertIn('A TSIG key with the name ', data['error'])

    def test_post_broken_key_name(self):
        payload = {
            'name': unique_tsigkey_name(),
            'key': 'f\u0333oobar1======',
            'algorithm': 'hmac-md5'
        }
        r = self.session.post(self.url("/api/v1/servers/localhost/tsigkeys"),
                              headers={'accept': 'application/json'},
                              data=json.dumps(payload))
        self.assertEqual(r.status_code, 422)
        data = r.json()
        self.assertIn(' cannot be base64-decoded', data['error'])

    def test_post_wrong_algo(self):
        payload = {
            'name': unique_tsigkey_name(),
            'algorithm': 'foobar'
        }
        r = self.session.post(self.url("/api/v1/servers/localhost/tsigkeys"),
                              headers={'accept': 'application/json'},
                              data=json.dumps(payload))
        self.assertEqual(r.status_code, 400)
        data = r.json()
        self.assertIn('Invalid TSIG algorithm: ', data['error'])
