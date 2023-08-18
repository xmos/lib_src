@Library('xmos_jenkins_shared_library@v0.25.0') _

getApproval()

pipeline {
  agent {
    label 'x86_64&&macOS'
  }
  environment {
    REPO = 'lib_src'
    VIEW = getViewName(REPO)
  }
  options {
    skipDefaultCheckout()
  }
  parameters {
    string(
      name: 'TOOLS_VERSION',
      defaultValue: '15.2.1',
      description: 'The XTC tools version'
    )
  }
  environment {
    PYTHON_VERSION = "3.10.5"
    VENV_DIRNAME = ".venv"
    // Download to the same place where it gets built on a dev machine so it's easy
    // to replicate
    DOWNLOAD_DIRNAME = "build"
  }
  stages {
    stage('Get view') {
      steps {
        xcorePrepareSandbox("${VIEW}", "${REPO}")
      }
    }
    stage('Library checks') {
      steps {
        xcoreLibraryChecks("${REPO}")
      }
    }
    stage('xCORE builds') {
      steps {
        dir("${REPO}") {
          xcoreAllAppsBuild('examples')
          xcoreAllAppNotesBuild('examples')
          dir("${REPO}") {
            runXdoc('doc')
          }
        }
        // Archive all the generated .pdf docs
        archiveArtifacts artifacts: "${REPO}/**/pdf/*.pdf", fingerprint: true, allowEmptyArchive: true
      }
    }
    stage('Tests') {
      steps {
        withTools(params.TOOLS_VERSION) {
          withVenv {
            dir("tests") {
              localRunPytest('')
            }
          }
        }
      }
    }
  }
  post {
    success {
      updateViewfiles()
    }
    cleanup {
      xcoreCleanSandbox()
    }
  }
}
