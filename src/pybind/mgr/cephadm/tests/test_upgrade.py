import json
from unittest import mock

import pytest

from ceph.deployment.service_spec import PlacementSpec, ServiceSpec
from cephadm import CephadmOrchestrator
from cephadm.upgrade import CephadmUpgrade
from cephadm.serve import CephadmServe
from orchestrator import OrchestratorError
from .fixtures import _run_cephadm, wait, with_host, with_service


@mock.patch("cephadm.serve.CephadmServe._run_cephadm", _run_cephadm('{}'))
def test_upgrade_start(cephadm_module: CephadmOrchestrator):
    with with_host(cephadm_module, 'test'):
        with with_host(cephadm_module, 'test2'):
            with with_service(cephadm_module, ServiceSpec('mgr', placement=PlacementSpec(count=2))):
                assert wait(cephadm_module, cephadm_module.upgrade_start(
                    'image_id', None)) == 'Initiating upgrade to docker.io/image_id'

                assert wait(cephadm_module, cephadm_module.upgrade_status()
                            ).target_image == 'docker.io/image_id'

                assert wait(cephadm_module, cephadm_module.upgrade_pause()
                            ) == 'Paused upgrade to docker.io/image_id'

                assert wait(cephadm_module, cephadm_module.upgrade_resume()
                            ) == 'Resumed upgrade to docker.io/image_id'

                assert wait(cephadm_module, cephadm_module.upgrade_stop()
                            ) == 'Stopped upgrade to docker.io/image_id'


@mock.patch("cephadm.serve.CephadmServe._run_cephadm", _run_cephadm('{}'))
@pytest.mark.parametrize("use_repo_digest",
                         [
                             False,
                             True
                         ])
def test_upgrade_run(use_repo_digest, cephadm_module: CephadmOrchestrator):
    with with_host(cephadm_module, 'host1'):
        with with_host(cephadm_module, 'host2'):
            cephadm_module.set_container_image('global', 'from_image')
            cephadm_module.use_repo_digest = use_repo_digest
            with with_service(cephadm_module, ServiceSpec('mgr', placement=PlacementSpec(host_pattern='*', count=2)), CephadmOrchestrator.apply_mgr, ''),\
                mock.patch("cephadm.module.CephadmOrchestrator.lookup_release_name",
                           return_value='foo'),\
                mock.patch("cephadm.module.CephadmOrchestrator.version",
                           new_callable=mock.PropertyMock) as version_mock,\
                mock.patch("cephadm.module.CephadmOrchestrator.get",
                           return_value={
                               # capture fields in both mon and osd maps
                               "require_osd_release": "pacific",
                               "min_mon_release": 16,
                           }):
                version_mock.return_value = 'ceph version 18.2.1 (somehash)'
                assert wait(cephadm_module, cephadm_module.upgrade_start(
                    'to_image', None)) == 'Initiating upgrade to docker.io/to_image'

                assert wait(cephadm_module, cephadm_module.upgrade_status()
                            ).target_image == 'docker.io/to_image'

                def _versions_mock(cmd):
                    return json.dumps({
                        'mgr': {
                            'ceph version 1.2.3 (asdf) blah': 1
                        }
                    })

                cephadm_module._mon_command_mock_versions = _versions_mock

                with mock.patch("cephadm.serve.CephadmServe._run_cephadm", _run_cephadm(json.dumps({
                    'image_id': 'image_id',
                    'repo_digests': ['to_image@repo_digest'],
                    'ceph_version': 'ceph version 18.2.3 (hash)',
                }))):

                    cephadm_module.upgrade._do_upgrade()

                assert cephadm_module.upgrade_status is not None

                with mock.patch("cephadm.serve.CephadmServe._run_cephadm", _run_cephadm(
                    json.dumps([
                        dict(
                            name=list(cephadm_module.cache.daemons['host1'].keys())[0],
                            style='cephadm',
                            fsid='fsid',
                            container_id='container_id',
                            container_image_id='image_id',
                            container_image_digests=['to_image@repo_digest'],
                            deployed_by=['to_image@repo_digest'],
                            version='version',
                            state='running',
                        )
                    ])
                )):
                    CephadmServe(cephadm_module)._refresh_hosts_and_daemons()

                with mock.patch("cephadm.serve.CephadmServe._run_cephadm", _run_cephadm(json.dumps({
                    'image_id': 'image_id',
                    'repo_digests': ['to_image@repo_digest'],
                    'ceph_version': 'ceph version 18.2.3 (hash)',
                }))):
                    cephadm_module.upgrade._do_upgrade()

                _, image, _ = cephadm_module.check_mon_command({
                    'prefix': 'config get',
                    'who': 'global',
                    'key': 'container_image',
                })
                if use_repo_digest:
                    assert image == 'to_image@repo_digest'
                else:
                    assert image == 'docker.io/to_image'


def test_upgrade_state_null(cephadm_module: CephadmOrchestrator):
    # This test validates https://tracker.ceph.com/issues/47580
    cephadm_module.set_store('upgrade_state', 'null')
    CephadmUpgrade(cephadm_module)
    assert CephadmUpgrade(cephadm_module).upgrade_state is None


@mock.patch("cephadm.serve.CephadmServe._run_cephadm", _run_cephadm('{}'))
def test_not_enough_mgrs(cephadm_module: CephadmOrchestrator):
    with with_host(cephadm_module, 'host1'):
        with with_service(cephadm_module, ServiceSpec('mgr', placement=PlacementSpec(count=1)), CephadmOrchestrator.apply_mgr, ''):
            with pytest.raises(OrchestratorError):
                wait(cephadm_module, cephadm_module.upgrade_start('image_id', None))


@mock.patch("cephadm.serve.CephadmServe._run_cephadm", _run_cephadm('{}'))
@mock.patch("cephadm.CephadmOrchestrator.check_mon_command")
def test_enough_mons_for_ok_to_stop(check_mon_command, cephadm_module: CephadmOrchestrator):
    # only 2 monitors, not enough for ok-to-stop to ever pass
    check_mon_command.return_value = (
        0, '{"monmap": {"mons": [{"name": "mon.1"}, {"name": "mon.2"}]}}', '')
    assert not cephadm_module.upgrade._enough_mons_for_ok_to_stop()

    # 3 monitors, ok-to-stop should work fine
    check_mon_command.return_value = (
        0, '{"monmap": {"mons": [{"name": "mon.1"}, {"name": "mon.2"}, {"name": "mon.3"}]}}', '')
    assert cephadm_module.upgrade._enough_mons_for_ok_to_stop()


@mock.patch("cephadm.serve.CephadmServe._run_cephadm", _run_cephadm('{}'))
@mock.patch("cephadm.serve.CephadmServe._get_container_image_info")
@mock.patch("cephadm.CephadmOrchestrator.lookup_release_name")
@mock.patch("cephadm.upgrade.CephadmUpgrade._check_target_version")
@mock.patch("cephadm.upgrade.CephadmUpgrade.get_distinct_container_image_settings")
@mock.patch('cephadm.upgrade.CEPH_UPGRADE_ORDER', [])
def test_completion_message(get_distinct_container_image_settings, _check_target_version, lookup_release_name,
                            _get_container_image_info, cephadm_module: CephadmOrchestrator):
    with with_host(cephadm_module, 'host1'):
        with with_host(cephadm_module, 'host2'):
            with with_service(cephadm_module, ServiceSpec('mgr', placement=PlacementSpec(count=2)), CephadmOrchestrator.apply_mgr, ''):
                wait(cephadm_module, cephadm_module.upgrade_start('image_id', None))
                assert wait(cephadm_module, cephadm_module.upgrade_status()
                            ).message == ''

                _get_container_image_info.return_value = (
                    'image_id', 'ceph version 99.0.0 (abc)', ['image@digest'])
                _check_target_version.return_value = None

                # check upgrade completion message is set
                cephadm_module.upgrade._do_upgrade()
                msg = wait(cephadm_module, cephadm_module.upgrade_status()
                           ).message
                assert msg.startswith("Completed upgrade to docker.io/image_id at")

                # check upgrade completion message is removed when new upgrade is started
                wait(cephadm_module, cephadm_module.upgrade_start('image_id', None))
                msg = wait(cephadm_module, cephadm_module.upgrade_status()
                           ).message
                assert not msg.startswith("Completed")

                # check upgrade atopped message is set when upgrade is stopped
                wait(cephadm_module, cephadm_module.upgrade_stop())
                msg = wait(cephadm_module, cephadm_module.upgrade_status()
                           ).message
                assert msg.startswith("Stopped upgrade to docker.io/image_id at")
