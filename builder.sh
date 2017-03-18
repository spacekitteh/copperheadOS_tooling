#setup build environment
mkdir .repo
cp $localManifests .repo/local_manifests
cp $repoSrc .repo/repo

#setup repo and verify
repo init -u https://github.com/CopperheadOS/platform_manifest.git -b refs/tags/$buildTag --depth=1 -reference=$sourceMirror
gpg --recv-keys 65EEFE022108E2B708CBFCF7F9E712E59AF5F22A
gpg --recv-keys 4340D13570EF945E83810964E8AD3F819AB10E78
cd .repo/manifests.git
git verify-tag --raw $(git describe)
cd ../..

#check out source
repo sync --current-branch -j32 

#verify source
repo forall -c 'git verify-tag --raw $(git describe)' || (echo "Verification failed!"; exit 1)

source build/envsetup.sh
choosecombo $buildType aosp_$model $userType
make -j
