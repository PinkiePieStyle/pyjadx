/* Copyright 2018 R. Thomas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "core.hpp"
#include <dlfcn.h>
#include <numeric>
namespace jni {

jadx::api::JadxDecompiler Jadx::load(const std::string& apk_path,
      bool escape_unicode,
      bool show_inconsistent_code,
      bool deobfuscation_on,
      size_t deobfuscation_min_length,
      size_t deobfuscation_max_length,
      bool replace_consts)
{
  const std::lock_guard<std::mutex> lock(jvm_mu);
  jni::jadx::api::JadxArgs jadx_args{this->env()};

  jadx_args.setInputFiles({apk_path});
  jadx_args.escape_unicode(escape_unicode);
  jadx_args.show_inconsistent_code(show_inconsistent_code);

  jadx_args.deobfuscation_on(deobfuscation_on);
  jadx_args.deobfuscation_min_length(deobfuscation_min_length);
  jadx_args.deobfuscation_max_length(deobfuscation_max_length);
  jadx_args.replace_consts(replace_consts);

  jni::jadx::api::JadxDecompiler decompiler{this->env(), jadx_args};
  decompiler.load();

  return decompiler;
}

std::vector<std::string> get_potential_libjvm_paths() {
  std::vector<std::string> libjvm_potential_paths;

  std::vector<std::string> search_prefixes;
  std::vector<std::string> search_suffixes;
  std::string file_name;

// From heuristics
#ifdef __WIN32
  search_prefixes = {""};
  search_suffixes = {"/jre/bin/server", "/bin/server"};
  file_name = "jvm.dll";
#elif __APPLE__
  search_prefixes = {""};
  search_suffixes = {"", "/jre/lib/server", "/lib/server"};
  file_name = "libjvm.dylib";

// SFrame uses /usr/libexec/java_home to find JAVA_HOME; for now we are
// expecting users to set an environment variable
#else
  search_prefixes = {
      "/usr/lib/jvm/default-java",
      "/usr/lib/jvm/java",
      "/usr/lib/jvm",
      "/usr/lib64/jvm",

      "/usr/local/lib/jvm/default-java",
      "/usr/local/lib/jvm/java",
      "/usr/local/lib/jvm",
      "/usr/local/lib64/jvm",

      "/usr/local/lib/jvm/java-7-openjdk-amd64",
      "/usr/lib/jvm/java-7-openjdk-amd64",

      "/usr/local/lib/jvm/java-6-openjdk-amd64",
      "/usr/lib/jvm/java-6-openjdk-amd64",

      "/usr/lib/jvm/java-8-openjdk-amd64",
      "/usr/local/lib/jvm/java-8-openjdk-amd64",

      "/usr/lib/jvm/java-10-openjdk-amd64",
      "/usr/local/lib/jvm/java-10-openjdk-amd64",

      "/usr/lib/jvm/java-11-openjdk-amd64",
      "/usr/local/lib/jvm/java-11-openjdk-amd64",

      "/usr/lib/jvm/java-6-oracle",
      "/usr/lib/jvm/java-7-oracle",
      "/usr/lib/jvm/java-8-oracle",

      "/usr/local/lib/jvm/java-6-oracle",
      "/usr/local/lib/jvm/java-7-oracle",
      "/usr/local/lib/jvm/java-8-oracle",

      "/usr/lib/jvm/java-9-jdk",
      "/usr/local/lib/jvm/java-9-jdk",

      "/usr/lib/jvm/java-10-jdk",
      "/usr/local/lib/jvm/java-10-jdk",

      "/usr/lib/jvm/java-11-jdk",
      "/usr/local/lib/jvm/java-11-jdk",

      "/usr/lib/jvm/default",
      "/usr/lib/jvm/default-runtime",

      "/usr/java/latest",
  };
  search_suffixes = {"", "/jre/lib/amd64/server", "jre/lib/amd64", "lib/server", "/lib/amd64/server/"};
  file_name = "libjvm.so";
#endif
  // From direct environment variable
  char* env_value = NULL;
  if ((env_value = getenv("JAVA_HOME")) != NULL) {
    search_prefixes.insert(search_prefixes.begin(), env_value);
  }

  // Generate cross product between search_prefixes, search_suffixes, and file_name
  for (const std::string& prefix : search_prefixes) {
    for (const std::string& suffix : search_suffixes) {
      std::string path = prefix + "/" + suffix + "/" + file_name;
      libjvm_potential_paths.push_back(path);
    }
  }

  return libjvm_potential_paths;
}

int try_dlopen(std::vector<std::string> potential_paths, void*& out_handle) {
  for (const std::string& i : potential_paths) {
    out_handle = dlopen(i.c_str(), RTLD_NOW | RTLD_LOCAL);

    if (out_handle != nullptr) {
      break;
    }
  }

  if (out_handle == nullptr) {
    return 0;
  }
  return 1;
}

bool resolve(JNI_CreateJavaVM_t& hdl, JNI_GetCreatedJavaVMs_t& hdl2) {
  if constexpr (jvm_static_link) {
    hdl  = &JNI_CreateJavaVM;
    hdl2 = &JNI_GetCreatedJavaVMs;
    return true;
  } else {
    std::vector<std::string> paths = get_potential_libjvm_paths();
    void* handler = nullptr;
    if (try_dlopen(paths, handler)) {
      auto&& fptr = reinterpret_cast<JNI_CreateJavaVM_t>(dlsym(handler, "JNI_CreateJavaVM"));
      if (fptr != nullptr) {
        hdl = fptr;
      } else {
        return false;
      }

      auto&& fptr_get_created = reinterpret_cast<JNI_GetCreatedJavaVMs_t>(dlsym(handler, "JNI_GetCreatedJavaVMs"));
      if (fptr_get_created != nullptr) {
        hdl2 = fptr_get_created;
      } else {
        return false;
      }
      return true;
    }
    return false;
  }

}

Jadx& Jadx::instance() {
  const std::lock_guard<std::mutex> lock(jvm_mu);
  if (instance_ == nullptr) {
    instance_ = new Jadx{};
    std::atexit(destroy);
  }
  return *instance_;
}

void Jadx::destroy(void) {
  const std::lock_guard<std::mutex> lock(jvm_mu);
  delete instance_;
}

Jadx::Jadx(void) {
  JNI_CreateJavaVM_t jvm_fnc              = nullptr;
  JNI_GetCreatedJavaVMs_t jvm_get_created = nullptr;

  if (not resolve(jvm_fnc, jvm_get_created)) {
    std::cerr << "[-] Can't resolve JVM symbols" << std::endl;
    return;
  }

  static constexpr std::initializer_list<const char*> jadx_libraries = {
    "android-29-clst.jar",
    "android-29-res.jar",
    "annotations-18.0.0.jar",
    "antlr-2.7.7.jar",
    "antlr-runtime-3.5.2.jar",
    "apksig-3.5.3.jar",
    "asm-7.3.1.jar",
    "baksmali-2.4.0.jar",
    "checker-qual-2.10.0.jar",
    "commons-lang3-3.9.jar",
    "commons-text-1.8.jar",
    "dexlib2-2.4.0.jar",
    "dx-1.16.jar",
    "error_prone_annotations-2.3.4.jar",
    "failureaccess-1.0.1.jar",
    "gson-2.8.6.jar",
    "guava-28.2-jre.jar",
    "image-viewer-1.2.3.jar",
    "j2objc-annotations-1.3.jar",
    "jadx-cli-dev.jar",
    "jadx-core-dev.jar",
    "jadx-gui-dev.jar",
    "jcommander-1.78.jar",
    "jfontchooser-1.0.5.jar",
    "jsr305-3.0.2.jar",
    "listenablefuture-9999.0-empty-to-avoid-conflict-with-guava.jar",
    "logback-classic-1.2.3.jar",
    "logback-core-1.2.3.jar",
    "reactive-streams-1.0.3.jar",
    "rsyntaxtextarea-3.0.8.jar",
    "rxjava-2.2.17.jar",
    "rxjava2-swing-0.3.7.jar",
    "slf4j-api-1.7.30.jar",
    "smali-2.4.0.jar",
    "stringtemplate-3.2.1.jar",
    "util-2.4.0.jar",
  };

  static const std::string prefix = get_jadx_prefix() + "/jadx/47dadf0/";

  std::string classpath = std::accumulate(
      std::begin(jadx_libraries),
      std::end(jadx_libraries),
      std::string{},
      [] (const std::string& lhs, const char* rhs) {
        return lhs.empty() ? prefix + rhs : lhs + ":" + prefix + rhs;
      });

  std::string classpath_option = "-Djava.class.path=" + classpath;


  // 1. Get the number of JVM created
  int nJVMs;
  jvm_get_created(nullptr, 0, &nJVMs);
  if (nJVMs > 0) {
    jvm_get_created(&this->jvm_, 1, &nJVMs);
    if (this->jvm_ == nullptr) {
      std::cerr << "Error while creating JVM" << std::endl;
      return;
    }
    if (this->jvm_->GetEnv(reinterpret_cast<void**>(&this->env_), JNI_VERSION_1_6) == JNI_EDETACHED) {
      this->jvm_->AttachCurrentThread(reinterpret_cast<void**>(&this->env_), nullptr);
      this->should_detach_ = true;
    }
    return;
  }

  JavaVMOption opt[1];
  opt[0].optionString = const_cast<char*>(classpath_option.c_str());

  JavaVMInitArgs args;
  args.version = JNI_VERSION_1_6;
  args.options = opt;
  args.nOptions = 1;
  args.ignoreUnrecognized = JNI_FALSE;

  int created = jvm_fnc(&(this->jvm_), reinterpret_cast<void**>(&this->env_), &args);

  // It mays occurs if a JVM is already created
  if (created != JNI_OK or this->jvm_ == nullptr) {
    std::cerr << "[-] Error while creating the JVM" << std::endl;
    std::abort();
  }

  //this->jvm_->AttachCurrentThread(reinterpret_cast<void**>(&this->env_), nullptr);
}

Jadx::~Jadx(void) {
  if (this->jvm_) {
    if (this->should_detach_) {
      this->jvm_->DetachCurrentThread();
    }
    //this->jvm_->DestroyJavaVM();
  }
}


}
